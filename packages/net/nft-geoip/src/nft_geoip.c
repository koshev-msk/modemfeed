// SPDX-License-Identifier: GPL-2.0-only
/*
 * nft_geoip - nftables geoip expression on top of the xt_geoip database.
 *
 * Reads /usr/share/xt_geoip/<CC>.iv4 and <CC>.iv6 (as produced by
 * maxminddb-dump-country / xt_geoip_build) directly from the kernel.
 *
 * File format, endianness independent (network byte order on disk):
 *   .iv4: array of { __be32 begin, end; }
 *   .iv6: array of { struct in6_addr begin, end; }
 * Ranges are sorted and non-overlapping, end is inclusive, no header.
 * The IPv4 ranges are converted to host order once, right after loading.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/refcount.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/kernel_read_file.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_tables.h>
#include <net/netfilter/nf_tables.h>

#include "nft_geoip.h"

#define GEOIP_DIR	"/usr/share/xt_geoip"

struct geoip_subnet4 {
	u32 begin, end;		/* big endian on disk, host order in memory */
};

struct geoip_subnet6 {
	struct in6_addr begin, end;
};

/* on-disk records must have no padding on any ABI */
static_assert(sizeof(struct geoip_subnet4) == 8);
static_assert(sizeof(struct geoip_subnet6) == 32);

struct geoip_country {
	struct list_head	list;
	refcount_t		ref;
	u16			cc;	/* ('R' << 8) | 'U' */
	struct geoip_subnet4	*s4;
	u32			n4;
	struct geoip_subnet6	*s6;
	u32			n6;
};

static LIST_HEAD(geoip_countries);
static DEFINE_MUTEX(geoip_lock);

/* ---------- database loading ---------- */

static int geoip_read(u16 cc, const char *ext, void **buf, size_t *size)
{
	char path[64];
	ssize_t ret;

	snprintf(path, sizeof(path), GEOIP_DIR "/%c%c.%s",
		 cc >> 8, cc & 0xff, ext);
	*buf = NULL;

	/*
	 * kernel_read_file_from_path() returns the number of bytes read on
	 * success, not 0. Callers here test for "non-zero == failure" and
	 * pass the value to ERR_PTR(), so only ever hand back 0 or -errno.
	 */
	ret = kernel_read_file_from_path(path, 0, buf, INT_MAX, size,
					 READING_UNKNOWN);
	return ret < 0 ? (int)ret : 0;
}

static void geoip_fixup4(struct geoip_subnet4 *s, u32 n)
{
	u32 i;

	for (i = 0; i < n; i++) {
		s[i].begin = be32_to_cpu((__force __be32)s[i].begin);
		s[i].end   = be32_to_cpu((__force __be32)s[i].end);
	}
}

static struct geoip_country *geoip_load(u16 cc)
{
	struct geoip_country *c;
	size_t sz4 = 0, sz6 = 0;
	int err4, err6;

	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return ERR_PTR(-ENOMEM);

	c->cc = cc;
	refcount_set(&c->ref, 1);

	err4 = geoip_read(cc, "iv4", (void **)&c->s4, &sz4);
	err6 = geoip_read(cc, "iv6", (void **)&c->s6, &sz6);

	/* a country may legitimately have only one of the two families */
	if (err4 && err6) {
		pr_warn("%c%c: cannot read database from %s (iv4: %d, iv6: %d)\n",
			cc >> 8, cc & 0xff, GEOIP_DIR, err4, err6);
		kfree(c);
		return ERR_PTR(err4 < 0 ? err4 : -EIO);
	}

	/* a missing file is fine, any other error is worth a message */
	if (err4 && err4 != -ENOENT)
		pr_warn("%c%c.iv4: read failed: %d\n", cc >> 8, cc & 0xff, err4);
	if (err6 && err6 != -ENOENT)
		pr_warn("%c%c.iv6: read failed: %d\n", cc >> 8, cc & 0xff, err6);

	if (err4) {
		c->s4 = NULL;
		sz4 = 0;
	}
	if (err6) {
		c->s6 = NULL;
		sz6 = 0;
	}

	if (sz4 % sizeof(struct geoip_subnet4) ||
	    sz6 % sizeof(struct geoip_subnet6)) {
		pr_warn("%c%c: bad database size (iv4: %zu, iv6: %zu)\n",
			cc >> 8, cc & 0xff, sz4, sz6);
		vfree(c->s4);
		vfree(c->s6);
		kfree(c);
		return ERR_PTR(-EINVAL);
	}

	c->n4 = sz4 / sizeof(struct geoip_subnet4);
	c->n6 = sz6 / sizeof(struct geoip_subnet6);
	geoip_fixup4(c->s4, c->n4);
	return c;
}

static struct geoip_country *geoip_get(u16 cc)
{
	struct geoip_country *c;

	mutex_lock(&geoip_lock);
	list_for_each_entry(c, &geoip_countries, list) {
		if (c->cc == cc) {
			refcount_inc(&c->ref);
			goto out;
		}
	}

	c = geoip_load(cc);
	if (!IS_ERR(c))
		list_add_tail(&c->list, &geoip_countries);
out:
	mutex_unlock(&geoip_lock);
	return c;
}

static void geoip_put(struct geoip_country *c)
{
	mutex_lock(&geoip_lock);
	if (refcount_dec_and_test(&c->ref)) {
		list_del(&c->list);
		vfree(c->s4);
		vfree(c->s6);
		kfree(c);
	}
	mutex_unlock(&geoip_lock);
}

/* ---------- lookup ---------- */

static bool geoip_match4(const struct geoip_country *c, __be32 addr)
{
	u32 ip = ntohl(addr);
	int lo = 0, hi = (int)c->n4 - 1;

	while (lo <= hi) {
		int mid = lo + (hi - lo) / 2;
		const struct geoip_subnet4 *s = &c->s4[mid];

		if (ip < s->begin)
			hi = mid - 1;
		else if (ip > s->end)
			lo = mid + 1;
		else
			return true;
	}
	return false;
}

static bool geoip_match6(const struct geoip_country *c,
			 const struct in6_addr *addr)
{
	int lo = 0, hi = (int)c->n6 - 1;

	while (lo <= hi) {
		int mid = lo + (hi - lo) / 2;
		const struct geoip_subnet6 *s = &c->s6[mid];

		if (memcmp(addr, &s->begin, sizeof(*addr)) < 0)
			hi = mid - 1;
		else if (memcmp(addr, &s->end, sizeof(*addr)) > 0)
			lo = mid + 1;
		else
			return true;
	}
	return false;
}

/* ---------- expression ---------- */

struct nft_geoip {
	u32			flags;
	u8			count;
	struct geoip_country	*c[NFT_GEOIP_MAX];
};

static void nft_geoip_eval(const struct nft_expr *expr,
			   struct nft_regs *regs,
			   const struct nft_pktinfo *pkt)
{
	const struct nft_geoip *priv = nft_expr_priv(expr);
	bool dst = priv->flags & NFT_GEOIP_F_DST;
	bool hit = false;
	unsigned int i;

	switch (nft_pf(pkt)) {
	case NFPROTO_IPV4: {
		const struct iphdr *iph = ip_hdr(pkt->skb);
		__be32 a = dst ? iph->daddr : iph->saddr;

		for (i = 0; i < priv->count && !hit; i++)
			hit = geoip_match4(priv->c[i], a);
		break;
	}
	case NFPROTO_IPV6: {
		const struct ipv6hdr *ip6h = ipv6_hdr(pkt->skb);
		const struct in6_addr *a = dst ? &ip6h->daddr : &ip6h->saddr;

		for (i = 0; i < priv->count && !hit; i++)
			hit = geoip_match6(priv->c[i], a);
		break;
	}
	default:
		regs->verdict.code = NFT_BREAK;
		return;
	}

	if (hit ^ !!(priv->flags & NFT_GEOIP_F_INVERT))
		return;		/* match: continue with the next expression */

	regs->verdict.code = NFT_BREAK;
}

static const struct nla_policy nft_geoip_policy[NFTA_GEOIP_MAX + 1] = {
	[NFTA_GEOIP_FLAGS]	= { .type = NLA_U32 },
	[NFTA_GEOIP_CC]		= { .type = NLA_BINARY,
				    .len  = NFT_GEOIP_MAX * 2 },
};

static int nft_geoip_init(const struct nft_ctx *ctx,
			  const struct nft_expr *expr,
			  const struct nlattr * const tb[])
{
	struct nft_geoip *priv = nft_expr_priv(expr);
	const u8 *cc;
	int len, i;

	if (!tb[NFTA_GEOIP_CC])
		return -EINVAL;

	len = nla_len(tb[NFTA_GEOIP_CC]);
	if (len < 2 || len % 2)
		return -EINVAL;

	priv->flags = tb[NFTA_GEOIP_FLAGS] ?
		      ntohl(nla_get_be32(tb[NFTA_GEOIP_FLAGS])) : 0;
	priv->count = 0;

	cc = nla_data(tb[NFTA_GEOIP_CC]);
	for (i = 0; i < len / 2; i++) {
		struct geoip_country *c;
		u8 a = toupper(cc[2 * i]), b = toupper(cc[2 * i + 1]);

		if (!isalnum(a) || !isalnum(b)) {
			while (priv->count)
				geoip_put(priv->c[--priv->count]);
			return -EINVAL;
		}

		c = geoip_get((a << 8) | b);
		if (IS_ERR(c)) {
			while (priv->count)
				geoip_put(priv->c[--priv->count]);
			return PTR_ERR(c);
		}
		priv->c[priv->count++] = c;
	}
	return 0;
}

static void nft_geoip_destroy(const struct nft_ctx *ctx,
			      const struct nft_expr *expr)
{
	struct nft_geoip *priv = nft_expr_priv(expr);

	while (priv->count)
		geoip_put(priv->c[--priv->count]);
}

static int nft_geoip_dump(struct sk_buff *skb, const struct nft_expr *expr,
			  bool reset)
{
	const struct nft_geoip *priv = nft_expr_priv(expr);
	u8 cc[NFT_GEOIP_MAX * 2];
	unsigned int i;

	for (i = 0; i < priv->count; i++) {
		cc[2 * i]     = priv->c[i]->cc >> 8;
		cc[2 * i + 1] = priv->c[i]->cc & 0xff;
	}

	if (nla_put_be32(skb, NFTA_GEOIP_FLAGS, htonl(priv->flags)) ||
	    nla_put(skb, NFTA_GEOIP_CC, priv->count * 2, cc))
		return -1;
	return 0;
}

static struct nft_expr_type nft_geoip_type;

static const struct nft_expr_ops nft_geoip_ops = {
	.type		= &nft_geoip_type,
	.size		= NFT_EXPR_SIZE(sizeof(struct nft_geoip)),
	.eval		= nft_geoip_eval,
	.init		= nft_geoip_init,
	.destroy	= nft_geoip_destroy,
	.dump		= nft_geoip_dump,
#ifdef NFT_REDUCE_READONLY
	.reduce		= NFT_REDUCE_READONLY,
#endif
};

static struct nft_expr_type nft_geoip_type __read_mostly = {
	.name		= "geoip",
	.ops		= &nft_geoip_ops,
	.policy		= nft_geoip_policy,
	.maxattr	= NFTA_GEOIP_MAX,
	.owner		= THIS_MODULE,
};

static int __init nft_geoip_module_init(void)
{
	return nft_register_expr(&nft_geoip_type);
}

static void __exit nft_geoip_module_exit(void)
{
	nft_unregister_expr(&nft_geoip_type);
}

module_init(nft_geoip_module_init);
module_exit(nft_geoip_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("nftables geoip expression using the xt_geoip database");
MODULE_ALIAS_NFT_EXPR("geoip");
MODULE_AUTHOR("Konstantine Shevlakov <kosnt.shell@gmail.com>");

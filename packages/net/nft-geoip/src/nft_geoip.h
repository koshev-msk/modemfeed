#ifndef _NFT_GEOIP_H
#define _NFT_GEOIP_H

#define NFT_GEOIP_MAX	16	/* countries per expression */

enum nft_geoip_flags {
	NFT_GEOIP_F_DST		= (1 << 0),	/* match daddr instead of saddr */
	NFT_GEOIP_F_INVERT	= (1 << 1),	/* geoip ... != cc */
};

enum nft_geoip_attributes {
	NFTA_GEOIP_UNSPEC,
	NFTA_GEOIP_FLAGS,	/* be32, enum nft_geoip_flags */
	NFTA_GEOIP_CC,		/* binary, N x 2 ASCII chars, N <= NFT_GEOIP_MAX */
	__NFTA_GEOIP_MAX
};
#define NFTA_GEOIP_MAX	(__NFTA_GEOIP_MAX - 1)

#endif /* _NFT_GEOIP_H */

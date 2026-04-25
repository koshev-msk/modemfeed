#!/bin/sh

# teleproxy config updater
# by koshev-msk 2026

URL=https://core.telegram.org/

which curl || echo "Error. cURL not found."

# Check proxy connectrion 
SOCKS_PROXY=$(uci -q get teleproxy.default.socks)
[ -n $SOCKS_PROXY ] && {
	# Check autentification proxy
	SOCKS_AUTH=$(uci -q get teleproxy.default.socks_auth)
	[ -n $SOCKS_AUTH ] && {
		CURL="curl -x socks5://${SOCKS_AUTH}@${SOCKS_PROXY}"
	} || {
		CURL="curl -x socks://${SOCKS_PROXY}"
	}
} || {
	CURL="curl"
}

[ -d /etc/teleproxy ] || mkdir -p /etc/teleproxy

# Download latest configs and check new sha256
${CURL} -s --max-time 60 $URL/cidr.txt -o /tmp/cidr.txt && \
	SHA256_CIDR_NEW=$(sha256sum /tmp/cidr.txt | awk '{print $1}') || SHA256_CIDR_NEW="failed"
${CURL} -s --max-time 60 $URL/getProxyConfig -o /tmp/proxy-multi.conf && \
	SHA256_MULTI_NEW=$(sha256sum /tmp/proxy-multi.conf | awk '{print $1}') || SHA256_MULTI_NEW="failed"
${CURL} -s --max-time 60 $URL/getProxySecret -o /tmp/aes-secret && \
	SHA256_AES_NEW=$(sha256sum /tmp/aes-secret | awk '{print $1}') || SHA256_AES_NEW="failed"

# Check old sha256 config
[ -f /etc/teleproxy/cidr.txt ] && \
	SHA256_CIDR_OLD=$(sha256sum /etc/teleproxy/cidr.txt | awk '{print $1}') || SHA256_CIDR_OLD="failed"
[ -f /etc/teleproxy/proxy-multi.conf ] && \
	SHA256_MULTI_OLD=$(sha256sum /etc/teleproxy/proxy-multi.conf | awk '{print $1}') || SHA256_MULTI_OLD="failed"
[ -f /etc/teleproxy/aes-secret ] && \
	SHA256_AES_OLD=$(sha256sum /etc/teleproxy/aes-secret | awk '{print $1}') || SHA256_MULTI_OLD="failed"

# Comapre hashes
[ "$SHA256_CIDR_NEW" = "$SHA256_CIDR_OLD" ] || \
	cp /tmp/cidr.txt /etc/teleproxy/cidr.txt && rm /tmp/cidr.txt
[ "$SHA256_MULTI_NEW" = "$SHA256_MULTI_OLD" ] || \
	cp /tmp/proxy-multi.conf /etc/teleproxy/proxy-multi.conf && rm /tmp/proxy-multi.conf
[ "$SHA256_AES_NEW" = "$SHA256_AES_OLD" ] || \
	cp /tmp/aes-secret /etc/teleproxy/aes-secret && rm /tmp/aes-secret

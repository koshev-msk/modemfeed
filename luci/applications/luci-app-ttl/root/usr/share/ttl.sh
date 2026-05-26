#!/bin/sh
. /lib/functions.sh

# handle_section is called by config_foreach for each UCI section of type "ttl".
# $1 = section name (e.g. "cfg1", or named section)
handle_section(){
	local s="$1"
	config_get method   "$s" method
	config_get advanced "$s" advanced
	config_get inet     "$s" inet
	config_get ports    "$s" ports
	config_get ttl      "$s" ttl    64
	config_get iface    "$s" iface
	config_get proxy    "$s" proxy

	[ -n "$iface" ] && ifn="$iface" || ifn="lan"
	DEV=$(ifstatus "$ifn" | jsonfilter -e '@["l3_device"]')

	case $method in
		ttl)   method_ttl   ;;
		proxy) method_proxy ;;
	esac
}

config_load ttl

# Choose firewall backend: nft takes priority over iptables
if [ -x /usr/sbin/nft ]; then
	. /usr/share/ttlnft.sh
elif [ -x /usr/sbin/iptables ] || [ -x /usr/sbin/ip6tables ]; then
	. /usr/share/ttlipt.sh
else
	logger -t ttl "No firewall backend found (nft/iptables/ip6tables)"
	exit 1
fi

config_foreach handle_section ttl

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

	case "$inet" in
		ipv4)  family="ip";     IPT="iptables" ;;
		ipv6)  family="ip6";    IPT="ip6tables" ;;
		*)     family="ip ip6"; IPT="iptables ip6tables" ;;
	esac

	[ -n "$iface" ] && ifn="$iface" || ifn="lan"
	DEV=$(ifstatus "$ifn" | jsonfilter -e '@["l3_device"]')

	case $method in
		ttl)   method_ttl   ;;
		proxy) method_proxy ;;
	esac
}

# --- Input validation ---
validate_ttl(){
        echo "$1" | grep -qE '^([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$' || {
                logger -t ttl "Invalid TTL value: '$1' (must be 0-255)"; exit 1
        }
}

validate_ports(){
        case "$1" in
                all|http|"") return 0 ;;
        esac
        echo "$1" | grep -qE '^[0-9]+(,[0-9]+)*$' || {
                logger -t ttl "Invalid ports value: '$1'"; exit 1
        }
}

validate_iface(){
        echo "$1" | grep -qE '^[a-zA-Z0-9._-]{1,15}$' || {
                logger -t ttl "Invalid iface value: '$1'"; exit 1
        }
}

validate_proxy(){
        echo "$1" | grep -qE '^(\[?[0-9a-fA-F:.]+\]?):([0-9]{1,5})$' || {
                logger -t ttl "Invalid proxy value: '$1'"; exit 1
        }
}
# --- End validation ---

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

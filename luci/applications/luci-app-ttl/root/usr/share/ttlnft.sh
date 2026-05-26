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

method_ttl(){

    ttl=${ttl:=64}
    validate_ttl "$ttl"
    [ -n "$iface" ] && validate_iface "$iface"

    # create mangle table
    nft add table ip mangle 2>/dev/null
    nft add table ip6 mangle 2>/dev/null

    for fam in $family; do
        [ "$fam" = "ip6" ] && [ "$inet" = "ipv4" ] && continue
        [ "$fam" = "ip" ] && [ "$inet" = "ipv6" ] && continue

        # define nftables chains
        nft add chain $fam mangle TTLFIX { type filter hook prerouting priority -150 \; }
        nft add chain $fam mangle TTL_OUT { type route hook output priority -150 \; }
        nft add chain $fam mangle TTL_POST { type filter hook postrouting priority -150 \; }

        # TTL/HL change rules
	case $fam in
		ip) TTLNAME=ttl ;;
		ip6) TTLNAME=hoplimit ;;
	esac
	if [ -n "$iface" ]; then
		nft add rule $fam mangle TTLFIX iif $DEV $fam $TTLNAME 1 $fam $TTLNAME set $ttl
		nft add rule $fam mangle TTL_OUT oif $DEV $fam $TTLNAME set $ttl
		nft add rule $fam mangle TTL_POST oif $DEV $fam $TTLNAME set $ttl
	else
		nft add rule $fam mangle TTLFIX $fam $TTLNAME 1 $fam $TTLNAME set $ttl
		nft add rule $fam mangle TTL_OUT $fam $TTLNAME set $ttl
		nft add rule $fam mangle TTL_POST $fam $TTLNAME set $ttl
	fi
    done
}

method_proxy(){
    validate_ports "$ports"
    [ -n "$proxy" ] && validate_proxy "$proxy"
    [ -n "$iface" ] && validate_iface "$iface"
    for fam in $family; do
	# create nat table
	nft add table $fam nat 2>/dev/null
        [ "$fam" = "ip6" ] && [ "$inet" = "ipv4" ] && continue
        [ "$fam" = "ip" ] && [ "$inet" = "ipv6" ] && continue

	[ "$proxy" ] && {
		IPADDR=${proxy%:*}
		END=${IPADDR}:${proxy#*:}
	} || {
	        # get ipaddress from iface if not defined
		case $fam in
	        	ip)
				IPADDR=$(ifstatus "$ifn" | jsonfilter -e '@["ipv4-address"][*]["address"]')
				END="${IPADDR}:3128"
			;;
			ip6)
				IPADDR=$(ifstatus "$ifn" | jsonfilter -e '@["ipv6-prefix-assignment"][*]["local-address"]["address"]' | head -n1)
				END="${IPADDR}:3128"
			;;
		esac
	}

        # create NAT chains
        nft add chain $fam nat PROXY { type nat hook prerouting priority -100 \; }
        nft add chain $fam nat FIXPROXY

        # add traffic rule
	[ -n "$iface" ] && {
	        nft add rule $fam nat PROXY iif $DEV jump FIXPROXY
	} || {
		nft add rule $fam nat PROXY jump FIXPROXY
	}


        case $ports in
            all)
                nft add rule $fam nat FIXPROXY $fam daddr != $IPADDR $fam saddr != $IPADDR \
                    meta l4proto tcp dnat to $END
                ;;
            http)
                nft add rule $fam nat FIXPROXY $fam daddr != $IPADDR $fam saddr != $IPADDR \
                    meta l4proto tcp tcp dport {80,443} dnat to $END
                ;;
            *)
                if [ $ports ]; then
                    nft add rule $fam nat FIXPROXY $fam daddr != $IPADDR $fam saddr != $IPADDR \
                        meta l4proto tcp tcp dport {$(echo $ports | tr ',' ',')} dnat to $END
                else
                    nft add rule $fam nat FIXPROXY $fam daddr != $IPADDR $fam saddr != $IPADDR \
                        meta l4proto tcp dnat to $END
                fi
                ;;
        esac
    done
}

# init tables and chains
for fml in ip ip6; do
    nft delete table $fml mangle 2>/dev/null
    nft delete table $fml nat 2>/dev/null

    nft add table $fml mangle
    nft add chain $fml mangle TTLFIX { type filter hook prerouting priority -150 \; }
    nft add chain $fml mangle TTL_OUT { type route hook output priority -150 \; }
    nft add chain $fml mangle TTL_POST { type filter hook postrouting priority -150 \; }
done

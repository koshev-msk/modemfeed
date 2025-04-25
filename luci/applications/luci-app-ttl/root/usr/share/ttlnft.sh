method_ttl(){
    if [ ! $ttl ]; then
        ttl=64
    fi
    case $(($ttl % 2)) in
        0) TTL_INC=4 ;;
        *) TTL_INC=5 ;;
    esac
    
    # create mangle table
    nft add table ip mangle 2>/dev/null
    nft add table ip6 mangle 2>/dev/null
    
    for family in ip ip6; do
        [ "$family" = "ip6" ] && [ "$inet" = "ipv4" ] && continue
        [ "$family" = "ip" ] && [ "$inet" = "ipv6" ] && continue
        
        # define nftables chains
        nft add chain $family mangle TTLFIX { type filter hook prerouting priority -150 \; }
        nft add chain $family mangle TTL_OUT { type route hook output priority -150 \; }
        nft add chain $family mangle TTL_POST { type filter hook postrouting priority -150 \; }
        
        # TTL/HL change rules
        if [ "$family" = "ip" ]; then
	    # Not supported yet
            #if [ $iface ]; then
            #    nft add rule $family mangle TTLFIX iif $DEV ip ttl 1 ip ttl inc $TTL_INC
            #else
            #    nft add rule $family mangle TTLFIX ip ttl 1 ip ttl inc $TTL_INC
            #fi
            nft add rule $family mangle TTL_OUT oif $DEV ip ttl set $ttl
            nft add rule $family mangle TTL_POST oif $DEV ip ttl set $ttl
        else
	    # not supported yet
            #if [ $iface ]; then
            #    nft add rule $family mangle TTLFIX iif $DEV ip6 hoplimit 1 ip6 hoplimit inc $TTL_INC
            #else
            #    nft add rule $family mangle TTLFIX ip6 hoplimit 1 ip6 hoplimit inc $TTL_INC
            #fi
            nft add rule $family mangle TTL_OUT oif $DEV ip6 hoplimit set $ttl
            nft add rule $family mangle TTL_POST oif $DEV ip6 hoplimit set $ttl
        fi
    done
}

method_proxy(){
    # Create NAT table
    nft add table ip nat 2>/dev/null
    nft add table ip6 nat 2>/dev/null
    
    for family in ip ip6; do
        [ "$family" = "ip6" ] && [ "$inet" = "ipv4" ] && continue
        [ "$family" = "ip" ] && [ "$inet" = "ipv6" ] && continue
        
        # get ipaddress from iface
        if [ "$family" = "ip" ]; then
            IPADDR=$(ifstatus $iface | jsonfilter -e '@["ipv4-address"][*]["address"]')
            END="${IPADDR}:3128"
        else
            IPADDR=$(ifstatus $iface | jsonfilter -e '@["ipv6-prefix-assignment"][*]["local-address"]["address"]' | head -n1)
            END="[$IPADDR]:3128"
        fi
        
        # create NAT chains
        nft add chain $family nat PROXY { type nat hook prerouting priority -100 \; }
        nft add chain $family nat FIXPROXY
        
        # add traffic rule
        nft add rule $family nat PROXY iif $DEV jump FIXPROXY
        
        case $ports in
            all)
                nft add rule $family nat FIXPROXY ip daddr != $IPADDR ip saddr != $IPADDR \
                    meta l4proto tcp dnat to $END
                ;;
            http)
                nft add rule $family nat FIXPROXY ip daddr != $IPADDR ip saddr != $IPADDR \
                    meta l4proto tcp tcp dport {80,443} dnat to $END
                ;;
            *)
                if [ $ports ]; then
                    nft add rule $family nat FIXPROXY ip daddr != $IPADDR ip saddr != $IPADDR \
                        meta l4proto tcp tcp dport {$(echo $ports | tr ',' ',')} dnat to $END
                else
                    nft add rule $family nat FIXPROXY ip daddr != $IPADDR ip saddr != $IPADDR \
                        meta l4proto tcp dnat to $END
                fi
                ;;
        esac
    done
}

# init tables and chains
for family in ip ip6; do
    # create mangle mangle
    nft delete table $family mangle 2>/dev/null
    
    # define chains
    nft add table $family mangle
    nft add chain $family mangle TTLFIX { type filter hook prerouting priority -150 \; }
    nft add chain $family mangle TTL_OUT { type route hook output priority -150 \; }
    nft add chain $family mangle TTL_POST { type filter hook postrouting priority -150 \; }
done

for s in $SECTIONS; do
    if [ "$s" ]; then
        get_vars
    else
        exit 0
    fi
    
    case $inet in
        ipv4) family="ip" ;;
        ipv6) family="ip6" ;;
        *) family="ip ip6" ;;
    esac
    
    if [ ! -f /lib/modules/$(uname -r)/ip6table_nat.ko ]; then
        family="ip"
    fi
    
    DEV=$(ifstatus $iface | jsonfilter -e '@["l3_device"]')
    
    #case $method in
        #ttl) method_ttl ;;
        # proxy) method_proxy ;;
    #esac
	method_ttl

done


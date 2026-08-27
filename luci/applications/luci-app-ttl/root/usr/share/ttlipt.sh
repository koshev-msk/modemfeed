# IPTABLES backend

method_ttl(){

	ttl=${ttl:=64}
	validate_ttl "$ttl"
	[ -n "$iface" ] && validate_iface "$iface"

	TTL_INC=$(($ttl-1))

	for T in $IPT; do
		case $T in
			iptables)
				SUFFIX="TTL --ttl-set"
				if [ -n "$iface" ]; then
					$T -t mangle -A TTLFIX -i $DEV -m ttl --ttl 1 -j TTL --ttl-inc $TTL_INC
				else
					$T -t mangle -A TTLFIX -m ttl --ttl 1 -j TTL --ttl-inc $TTL_INC
				fi
			;;
			ip6tables)
				SUFFIX="HL --hl-set"
				if [ -n "$iface" ]; then
					$T -t mangle -A TTLFIX -i $DEV -m hl --hl 1 -j HL --hl-inc $TTL_INC
				else
					$T -t mangle -A TTLFIX -m hl --hl 1 -j HL --hl-inc $TTL_INC
				fi
			;;
		esac

		if [ -n "$iface" ]; then
			$T -t mangle -A TTL_OUT -o $DEV -j $SUFFIX $ttl
			$T -t mangle -A TTL_POST -o $DEV -j $SUFFIX $ttl
		else
			$T -t mangle -A TTL_OUT -j $SUFFIX $ttl
			$T -t mangle -A TTL_POST -j $SUFFIX $ttl
		fi
	done
}


method_proxy(){
	validate_ports "$ports"
	[ -n "$proxy" ] && validate_proxy "$proxy"
	[ -n "$iface" ] && validate_iface "$iface"

	# check nat66 module
	[ -f /lib/modules/$(uname -r)/ip6table_nat.ko ] || IPT="iptables"

	for T in $IPT; do
		[ "$proxy" ] && {
			IPADDR=${proxy%:*}
			case $T in
				iptables) END=${IPADDR}:${proxy#*:} ;;
				ip6tables) END="[${IPADDR}]:${proxy#*:}" ;;
			esac
	        } || {
			case $T in
				iptables)
					IPADDR=$(ifstatus "$ifn" | jsonfilter -e '@["ipv4-address"][*]["address"]')
					END="${IPADDR}:3128"
				;;
				ip6tables)
					for a in $(ifstatus "$ifn" | jsonfilter -e '@["ipv6-prefix-assignment"][*]["local-address"]["address"]'); do
						IPADDR="$a"
					done
					END="[$IPADDR]:3128"
				;;
			esac
		}

		$T -t nat -A PROXY -i $DEV -j FIXPROXY

		case $ports in
			all)
				$T -t nat -A FIXPROXY ! -d ${IPADDR} \
					! -s ${IPADDR} -p tcp \
					-j DNAT --to-destination $END
			;;
			http)
				$T -t nat -A FIXPROXY ! -d ${IPADDR} \
					! -s ${IPADDR} -p tcp -m multiport \
					--dports 80,443 -j DNAT --to-destination $END
			;;
			*)
				if [ -n "$ports" ]; then
					$T -t nat -A FIXPROXY ! -d ${IPADDR} \
						! -s ${IPADDR} -p tcp -m multiport \
						--dports $ports -j DNAT --to-destination $END
				else
					$T -t nat -A FIXPROXY ! -d ${IPADDR} \
						! -s ${IPADDR} -p tcp \
						-j DNAT --to-destination $END
				fi
			;;
		esac
	done
}
	
# Create and flush mangle table
for T in $IPT; do
	for t in N F; do
		for c in TTLFIX TTL_OUT TTL_POST; do
			$T -t mangle -${t} ${c}
		done
	done
	for a in D I; do
		$T -t mangle -${a} PREROUTING -j TTLFIX
		$T -t mangle -${a} OUTPUT -j TTL_OUT
		$T -t mangle -${a} POSTROUTING -j TTL_POST
	done
done

# Create and flush nat table
for T in $IPT; do
	for t in N F; do
		$T -t nat -${t} PROXY
		$T -t nat -${t} FIXPROXY
	done
	for a in D I; do
		$T -t nat -${a} PREROUTING -j PROXY
	done
done

# --- Input validation ---
validate_ttl(){
	echo "$1" | grep -qE '^([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$' || {
		logger -t ttl "Invalid TTL value: '$1' (must be 0-255)"; exit 1
	}
}

validate_ports(){
	# accepts: "all", "http", or comma-separated port numbers
	case "$1" in
		all|http|"") return 0 ;;
	esac
	echo "$1" | grep -qE '^[0-9]+(,[0-9]+)*$' || {
		logger -t ttl "Invalid ports value: '$1'"; exit 1
	}
}

validate_iface(){
	# interface name: alphanumeric, dash, dot, underscore, max 15 chars
	echo "$1" | grep -qE '^[a-zA-Z0-9._-]{1,15}$' || {
		logger -t ttl "Invalid iface value: '$1'"; exit 1
	}
}

validate_proxy(){
	# IP:port or [IPv6]:port
	echo "$1" | grep -qE '^(\[?[0-9a-fA-F:.]+\]?):([0-9]{1,5})$' || {
		logger -t ttl "Invalid proxy value: '$1'"; exit 1
	}
}
# --- End validation ---

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

# check nat66 module
if [ -f /lib/modules/$(uname -r)/ip6table_nat.ko ]; then
	IPT="iptables ip6tables"
else
	IPT="iptables"
fi
	
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

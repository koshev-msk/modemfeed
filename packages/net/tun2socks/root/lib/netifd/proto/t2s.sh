#!/bin/sh

. /lib/functions.sh
. /lib/functions/network.sh
. ../netifd-proto.sh
init_proto "$@"

proto_t2s_init_config(){
	no_device=1
	available=1
	proto_config_add_string "network"
	proto_config_add_int "ip_manual"
	proto_config_add_string "ipaddr"
	proto_config_add_string "netmask"
	proto_config_add_string "gateway"
	proto_config_add_string "host"
	proto_config_add_string "proxy"
	proto_config_add_string "encrypt"
	proto_config_add_string "loglevel"
	proto_config_add_string "obfs_host"
	proto_config_add_string "username"
	proto_config_add_string "password"
	proto_config_add_string "opts"
	proto_config_add_string "sockpath"
	proto_config_add_int "mtu"
	proto_config_add_int "fwmark"
	proto_config_add_boolean "socket"
	proto_config_add_boolean "base64enc"
	proto_config_add_defaults
}

check_encrypt(){
	case $encrypt in
		"none"|\
		"table"|\
		"rc4"|\
		"rc4-md5"|\
		"aes-128-cfb"|\
		"aes-192-cfb"|\
		"aes-256-cfb"|\
		"aes-128-ctr"|\
		"aes-192-ctr"|\
		"aes-256-ctr"|\
		"aes-128-gcm"|\
		"aes-192-gcm"|\
		"aes-256-gcm"|\
		"camellia-128-cfb"|\
		"camellia-192-cfb"|\
		"camellia-256-cfb"|\
		"bf-cfb"|\
		"salsa20"|\
		"chacha20"|\
		"chacha20-ietf"|\
		"chacha20-ietf-poly1305"|\
		"xchacha20-ietf-poly1305")
				continue
		;;
		*)
			proto_notify_error "$interface" WRONG_ENCRYPT_METHOD
			proto_set_available "$interface" 0
		;;
	esac
}

getaddr() {
	local network="${1:-10.0.0.0/8}"

	awk -v network="$network" '
	BEGIN {
		srand()
		split(network, parts, "/")
		network_ip = parts[1]
		prefix = parts[2] == "" ? 24 : int(parts[2])

		split(network_ip, octets, ".")
		o1 = int(octets[1])
		o2 = int(octets[2])
		o3 = int(octets[3])
		o4 = int(octets[4])

		if (prefix <= 8) {
			r2 = int(rand() * 256)
		} else if (prefix <= 16) {
			bits = prefix - 8
			max = bits > 0 ? 2^(8 - bits) - 1 : 0
			r2 = o2 + int(rand() * (max + 1))
		} else {
			r2 = o2
		}

		if (prefix <= 16) {
			r3 = int(rand() * 256)
		} else if (prefix <= 24) {
			bits = prefix - 16
			max = bits > 0 ? 2^(8 - bits) - 1 : 0
			r3 = o3 + int(rand() * (max + 1))
		} else {
			r3 = o3
		}

		if (prefix <= 24) {
			base4 = int(rand() * 64) * 4
		} else {
			bits = prefix - 24
			max = bits > 0 ? 2^(8 - bits) - 1 : 0
			base4 = o4 + (int(rand() * (max + 1)) * 4)
			base4 = and(base4, 252)
			if (base4 > 252) base4 = 252
		}

		net_ip = o1 "." r2 "." r3 "." base4
		gw_ip = o1 "." r2 "." r3 "." (base4 + 1)
		dev_ip = o1 "." r2 "." r3 "." (base4 + 2)

		print "GENIPADDR=" dev_ip
		print "GENIPMASK=255.255.255.252"
		print "GENIPNET=" net_ip "/30"
		print "GENIPGW=" gw_ip
	}
	'
}


proto_t2s_setup(){
	local interface="$1"
	local network ifname ipaddr netmask gateway host proxy encrypt loglevel fwmark 
	local ip_manual base64enc socket obfs_host port mtu
	local username password opts sockpath $PROTO_DEFAULT_OPTIONS
	json_get_vars network ifname ipaddr netmask gateway host proxy encrypt loglevel fwmark
	json_get_vars ip_manual base64enc socket obfs_host port mtu
	json_get_vars username password opts sockpath $PROTO_DEFAULT_OPTIONS
	ifname=$interface
	[ "$metric" = "" ] && metric="0"
	[ "$proxy" = "" ] && proxy=socks5
	[ "$loglevel" = "" ] && loglevel=error
	[ "$host" ] && {
		case "$proxy" in
			http) ARGS="-proxy ${proxy}://${host}" ;;
			socks4)
				[ "$username" ] && {
					ARGS="-proxy ${proxy}://${username}@${host}"
				} || {
					ARGS="-proxy ${proxy}://${host}"
				}
			;;
			socks5)
				[ "$username" -a "$password" ] && {
					ARGS="-proxy ${proxy}://${username}:${password}@${host}"
				} || {
					ARGS="-proxy ${proxy}://${host}"
				}
			;;
			ss)
				#check_encrypt
				[ "$encrypt" -a "$password" ] && {
					[ "$base64enc" = "1" ] && {
						base64gen=$(echo ${encrypt}:${password} | base64)
						[ "$obfs_host" ] && {
							ARGS="-proxy ${proxy}://${base64gen}@${host}/\<\?obfs=http\;obfs-host=$obfs_host\>"
						} || {
							ARGS="-proxy ${proxy}://${base64gen}@${host}"
						}
					} || {
						[ "$obfs_host" ] && {
							ARGS="-proxy ${proxy}://${encrypt}:${password}@${host}/\<\?obfs=http\;obfs-host=$obfs_host\>"
						} || {
							ARGS="-proxy ${proxy}://${encrypt}:${password}@${host}"
						}
					}
				} || {
					proto_notify_error "$interface" CONFIGURE_FAILED
					proto_set_available "$interface" 0
				}
			;;
			relay)
				[ "$username" -a "$password" ] && {
					ARGS="-proxy ${proxy}://${encrypt}:${password}@${host}/\<nodelay=false\>"
				} || {
					ARGS="-proxy ${proxy}://${host}/\<nodelay=false\>"
				}
			;;
		esac
	}

	case $proxy in
		direct|reject) ARGS="-proxy ${proxy}://" ;;
		socks5)
			[ "$socket" ] &&  {
				[ "$sockpath" ] && {
					ARGS="-proxy ${proxy}://${sockpath}"
				} || {
					proto_notify_error "$interface" CONFIGURE_FAILED
					proto_set_available "$interface" 0
				}
			}
		;;
	esac

	[ "x${ARGS}" = "x" ] && {
		proto_notify_error "$interface" CONFIGURE_FAILED
		proto_set_available "$interface" 0
	}

	[ "$loglevel" ] && ARGS="$ARGS -loglevel $loglevel"
	[ "$fwmark" ] && ARGS="$ARGS -fwmark $fwmark"
	[ "$mtu" ] && ARGS="$ARGS -mtu $mtu"
	[ "$opts" ] && ARGS="$ARGS $opts"
	! [ "$ip_manual" = "1" ] && {
		eval $(getaddr $network)
		ipaddr=$GENIPADDR
		netmask=$GENIPMASK
		gateway=$GENIPGW
		echo "Assign $ipaddr mask $netmask gw $gateway"
	}

	proto_init_update "$interface" 1
	proto_add_data
	proto_close_data
	ip tuntap add mode tun dev $interface
	proto_set_keep 1
	proto_add_ipv4_address $ipaddr $netmask
	[ $gateway ] && {
		proto_add_ipv4_route "0.0.0.0" 0 $gateway $ipaddr
	}
	proto_add_data
	proto_close_data
	proto_send_update "$interface"
	proto_run_command "$interface" /usr/sbin/tun2socks \
		-device "$interface" $ARGS
}

proto_t2s_teardown(){
	local interface="$1"
	proto_kill_command "$interface"
	ip tuntap del mode tun dev $interface
}

[ -n "$INCLUDE_ONLY" ] || add_protocol t2s

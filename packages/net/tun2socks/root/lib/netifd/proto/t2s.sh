#!/bin/sh

. /lib/functions.sh
. /lib/functions/network.sh
. ../netifd-proto.sh
init_proto "$@"

proto_t2s_init_config(){
	no_device=1
	available=1
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
	proto_config_add_int "mtu"
	proto_config_add_int "port"
	proto_config_add_int "fwmark"
	proto_config_add_bool "socket"
	proto_config_add_bool "base64enc"
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

proto_t2s_setup(){
	local interface="$1"
	local ifname ipaddr netmask gateway host proxy encrypt loglevel fwmark 
	local base64enc socket obfs_host port mtu
	local username password opts $PROTO_DEFAULT_OPTIONS
	json_get_vars ifname ipaddr netmask gateway host proxy encrypt loglevel fwmark
	json_get_vars base64enc socket obfs_host port mtu
	json_get_vars username password opts $PROTO_DEFAULT_OPTIONS
	ifname=$interface
	[ "$metric" = "" ] && metric="0"
	[ "$proxy" = "" ] && proxy=socks5

	[ "$host" -a "$port" ] && {
		case "$proxy" in
			http) ARGS="-proxy ${proxy}://${host}:${port}" ;;
			socks4)
				[ "$username" ] && {
					ARGS="-proxy ${proxy}://${username}@${host}:${port}"
				} || {
					ARGS="-proxy ${proxy}://${host}:${port}"
				}
			;;
			socks5)
				[ "$username" -a "$password" ] && {
					ARGS="-proxy ${proxy}://${username}:${password}@${host}:${port}"
				} || {
					ARGS="-proxy ${proxy}://${host}:${port}"
				}
			;;
			ss)
				#check_encrypt
				[ "$encrypt" -a "$password" ] && {
					# TODO 
					#[ "$base64enc" = "1" ] && {
					#	[ "$obfs_host" ] && {
					#		ARGS="-proxy ${proxy}://base64_encode(${encrypt}:${password})@${host}:${port}/\<\?obfs=http\;obfs-host=$obfs_host\>"
					#	} || {
					#		ARGS="-proxy ${proxy}://base64_encode(${encrypt}:${password})@${host}:${port}"
					#	}
					#} || {
						[ "$obfs_host" ] && {
							ARGS="-proxy ${proxy}://${encrypt}:${password}@${host}:${port}/\<\?obfs=http\;obfs-host=$obfs_host\>"
						} || {
							ARGS="-proxy ${proxy}://${encrypt}:${password}@${host}:${port}"
						}
					#}
				} || {
					ARGS="-proxy ${proxy}://${host}:${port}"
				}
			;;
			relay)
				[ "$username" -a "$password" ] && {
					ARGS="-proxy ${proxy}://${encrypt}:${password}@${host}:${port}/\<nodelay=false\>"
				} || {
					ARGS="-proxy ${proxy}://${host}:${port}/\<nodelay=false\>"
				}
			;;
		esac
	}

	case $proxy in
		direct|reject) ARGS="-proxy ${proxy}://" ;;
		# TODO
		#socks5)
		#	[ "$socket" ] && {
		#		ARGS="-proxy ${proxy}://${socket}"
		#	}
		#;;
	esac

	[ "x${ARGS}" = "x" ] && {
		proto_notify_error "$interface" CONFIGURE_FAILED
		proto_set_available "$interface" 0
	}

	[ "$loglevel" = "" ] && loglevel=error && {
		ARGS="$ARGS -loglevel $loglevel"
	}

	[ "$fwmark" ] && {
		ARGS="$ARGS -fwmark $fwmark"
	}

	[ "$mtu" ] && {
		ARGS="$ARGS -mtu $mtu"
	}

	[ "$opts" ] && {
		 ARGS="$ARGS $opts"
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

add_protocol t2s

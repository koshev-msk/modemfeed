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
	proto_config_add_string "fwmark"
	proto_config_add_string "obfs_host"
	proto_config_add_int "mtu"
	proto_config_add_int "port"
	proto_config_add_string "username"
	proto_config_add_string "password"
	proto_config_add_string "opts"
	proto_config_add_defaults
}

proto_t2s_setup(){
	local interface="$1"
	local ifname ipaddr netmask gateway host proxy encrypt loglevel fwmark obfs_host port mtu username password opts $PROTO_DEFAULT_OPTIONS
	json_get_vars ifname ipaddr netmask gateway host proxy encrypt loglevel obfs_host fwmark port mtu username password opts $PROTO_DEFAULT_OPTIONS
	ifname=$interface
	[ "$metric" = "" ] && metric="0"
	[ "$proxy" = "" ] && proxy=socks5

	[ "$host" -a "$port" ] && {
		ARGS="-proxy ${proxy}://${host}:${port}"
	}
	[ "$host" -a "$port" -a "$username" -a "$password" ] && {
		ARGS="-proxy ${proxy}://${username}:${password}@${host}:${port}"
	}
	[ "$host" -a "$port" -a "$username" -a "$password" -a "$encrypt" ] && {
		ARGS="-proxy ${proxy}://${encrypt}:${username}:${password}@${host}:${port}"
	}
	
	case $proxy in
		direct|reject) ARGS="-proxy ${proxy}://" ;;
	esac

	[ "x${ARGS}" = "x" ] && {
		proto_notify_error "$interface" CONFIGURE_FAILED
	}

	[ "$loglevel" = "" ] && loglevel=error && {
		ARGS="$ARGS -loglevel $loglevel"
	}

	[ "$fwmark" ] && {
		ARGS="$ARGS -fwmark $fwmark"
	}

	[ "$opts" ] && {
		 ARGS="$ARGS $OPTS"
	}

	[ "$mtu" ] && {
		ARGS="$ARGS -mtu $mtu"
	}

	proto_init_update "$interface" 1
	proto_add_data
	proto_close_data
	ip tuntap add mode tun dev $interface
	ip link set dev $interface up
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

#!/bin/sh

[ -n "$INCLUDE_ONLY" ] || {
	. /lib/functions.sh
	. /lib/functions/network.sh
	. ../netifd-proto.sh
	init_proto "$@"
}

proto_h2s_init_config() {
	no_device=1
	available=1

        proto_config_add_array "ipaddr:list(string)"
        proto_config_add_array "gateway:list(string)"
	proto_config_add_string server
	proto_config_add_int port
	proto_config_add_string username
	proto_config_add_string password
	proto_config_add_string udp
	proto_config_add_int mtu
	proto_config_add_string log_level
	proto_config_add_int mark
	proto_config_add_boolean tcp_fast_open

	proto_config_add_defaults
}

# Write a YAML single-quoted scalar.
# In YAML, a single quote inside a single-quoted scalar is doubled.
h2s_yaml_quote() {
	printf "'"
	printf '%s' "$1" | sed "s/'/''/g"
	printf "'"
}

proto_h2s_setup() {
	local interface="$1"
	local server port username password
	local udp mtu log_level mark tcp_fast_open
	local ipaddrs gateways a ad m g src4
	local config i

	json_get_values ipaddrs ipaddr
	json_get_values gateways gateway
	json_get_var server server
	json_get_var port port
	json_get_var username username
	json_get_var password password
	json_get_var udp udp
	json_get_var mtu mtu
	json_get_var log_level log_level
	json_get_var mark mark
	json_get_var tcp_fast_open tcp_fast_open

	local ipaddr_type
	json_get_type ipaddr_type ipaddr

	# Hev uses the logical interface name as its TUN device name.
	if [ "${#interface}" -gt 15 ]; then
		proto_notify_error "$interface" INVALID_INTERFACE_NAME
		return 1
	fi

	case "$interface" in
		""|*[!a-zA-Z0-9_.-]*)
			proto_notify_error "$interface" INVALID_INTERFACE_NAME
			return 1
			;;
	esac

	[ -n "$server" ] || {
		proto_notify_error "$interface" SERVER_MISSING
		return 1
	}

	case "$port" in
		""|*[!0-9]*)
			proto_notify_error "$interface" INVALID_PORT
			return 1
			;;
	esac

	[ "$port" -ge 1 ] && [ "$port" -le 65535 ] || {
		proto_notify_error "$interface" INVALID_PORT
		return 1
	}

	if [ -n "$username" ] && [ -z "$password" ] ||
	   [ -z "$username" ] && [ -n "$password" ]; then
		proto_notify_error "$interface" INVALID_AUTH
		return 1
	fi

	[ -n "$udp" ] || udp="udp"
	case "$udp" in
		tcp|udp) ;;
		*)
			proto_notify_error "$interface" INVALID_UDP_MODE
			return 1
			;;
	esac

	[ -n "$mtu" ] || mtu=8500
	case "$mtu" in
		*[!0-9]*|"")
			proto_notify_error "$interface" INVALID_MTU
			return 1
			;;
	esac

	[ -n "$log_level" ] || log_level="warn"
	case "$log_level" in
		debug|info|warn|error) ;;
		*)
			proto_notify_error "$interface" INVALID_LOG_LEVEL
			return 1
			;;
	esac

	config="/var/run/hev-${interface}.yml"

	umask 077
	mkdir -p /var/run || {
		proto_notify_error "$interface" CONFIG_DIR_FAILED
		return 1
	}

	{
		cat <<EOF
tunnel:
  name: $(h2s_yaml_quote "$interface")
  mtu: $mtu

socks5:
  address: $(h2s_yaml_quote "$server")
  port: $port
  udp: $(h2s_yaml_quote "$udp")
EOF

		if [ -n "$username" ]; then
			printf '  username: '
			h2s_yaml_quote "$username"
			printf '\n  password: '
			h2s_yaml_quote "$password"
			printf '\n'
		fi

		if [ -n "$mark" ]; then
			case "$mark" in
				*[!0-9]*)
					exit 1
					;;
			esac
			printf '  mark: %s\n' "$mark"
		fi

		if [ "$tcp_fast_open" = "1" ]; then
			printf '  tcp-fastopen: true\n'
		fi

		cat <<EOF

misc:
  log-level: $(h2s_yaml_quote "$log_level")
EOF
	} > "$config" || {
		rm -f "$config"
		proto_notify_error "$interface" CONFIG_WRITE_FAILED
		return 1
	}

	# Hev must stay in foreground: do not configure misc.pid-file.
	proto_run_command "$interface" \
		/usr/bin/hev-socks5-tunnel "$config"

	# Wait until Hev creates its TUN device.
	i=0
	while [ "$i" -lt 50 ]; do
		[ -d "/sys/class/net/$interface" ] && break
		sleep 3
		i=$((i + 1))
	done

	if [ ! -d "/sys/class/net/$interface" ]; then
		proto_kill_command "$interface"
		rm -f "$config"
		proto_notify_error "$interface" TUN_CREATE_FAILED
		return 1
	fi

	proto_init_update "$interface" 1
	proto_set_keep 1

	for a in $ipaddrs; do
		ad="${a%%/*}"
		m="${a#*/}"
		case "$a" in
			*:*)
				[ "$m" = "$a" ] && m=128
				proto_add_ipv6_address "$ad" "$m"
			;;
			*)
				[ "$m" = "$a" ] && m=32
				proto_add_ipv4_address "$ad" "$m"
				[ "$src4" ] || src4=$ad
			;;
		esac
	done
	for g in $gateways; do
		case "$g" in
			*:*) proto_add_ipv6_route "::" 0 "$g" ;;
			*) proto_add_ipv4_route "0.0.0.0" 0 "$g" "$src4" ;;
		esac
	done

	proto_send_update "$interface"
}

proto_h2s_teardown() {
	local interface="$1"

	proto_kill_command "$interface"
	rm -f "/var/run/hev-${interface}.yml"
}

[ -n "$INCLUDE_ONLY" ] || add_protocol h2s


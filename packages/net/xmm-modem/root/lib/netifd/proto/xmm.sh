#!/bin/sh

# protocol handler for INTEL-like modem
# XMM7530 XMM7650 T700 chipsents
# by Konstatntine Shevlakov 2024-2025 <shevlakov@132lan.ru>

. /lib/functions.sh
. /lib/functions/network.sh
. ../netifd-proto.sh
init_proto "$@"


valid_ip4(){
	/bin/ipcalc.sh "${1}/${ip4mask}" > /dev/null 2>&1
}


proto_xmm_init_config() {
	no_device=1
	available=1
	proto_config_add_string "device:device"
	proto_config_add_string "apn"
	proto_config_add_string "pdp"
	proto_config_add_int "delay"
	proto_config_add_string "pincode"
	proto_config_add_string "username"
	proto_config_add_string "password"
	proto_config_add_string "auth"
	proto_config_add_int "profile"
	proto_config_add_int "maxfail"
	proto_config_add_defaults
}

proto_xmm_setup() {
	local interface="$1"
	local devname devpath hwaddr ip4addr ip4mask dns1 dns2 defroute lladdr
	local name ifname proto extendprefix auth username password
	local device ifname auth username password apn pdp profile maxfail pincode delay $PROTO_DEFAULT_OPTIONS
	json_get_vars device ifname auth username password apn pdp profile maxfail pincode delay $PROTO_DEFAULT_OPTIONS

	[ "$profile" = "" ] && profile="1"
	[ "$metric" = "" ] && metric="0"
	[ "$delay" = "" ] && delay="5"
	[ "$maxfail" = "" ] && maxfail="5"
	sleep $delay
	[ -z $ifname ] && {
		devname=$(basename $device)
		devpath="$(readlink -f /sys/class/tty/$devname/device)"
		case "$devname" in
			*ttyACM*|*ttyUSB*)
				if [ -r $(readlink -f /sys/class/tty/$devname/device/../idVendor) ]; then
					VID=$(cat $(readlink -f /sys/class/tty/$devname/device/../idVendor))
					PID=$(cat $(readlink -f /sys/class/tty/$devname/device/../idProduct))
				else
					VID=$(cat $(readlink -f /sys/class/tty/$devname/device/../../idVendor))
					PID=$(cat $(readlink -f /sys/class/tty/$devname/device/../../idProduct))
				fi
				VIDPID=$VID$PID
				case $VIDPID in
					8087095a)
						PREFIX="xmm"
						hwaddr="$(ls -1 $devpath/../*/net/*/*address*)"
						XMMDNS="XDNS"
					;;
					0e8d7126|0e8d7127)
						PREFIX="fm350"
						hwaddr="$(ls -1 $devpath/../../*/net/*/*address*)"
						XMMDNS="GTDNS"
					;;
					*)
						echo "Modem not supported!"
						proto_notify_error "$interface" NO_DEVICE_SUPPORT
						proto_set_available "$interface" 0
						return 1
					;;
				esac
			;;
			*)
				echo "AT port not valid!"
				proto_notify_error "$interface" NO_PORT_FOUND
			;;
		esac
		echo "Setup $PREFIX interface $interface with port ${device}"

		[ "${devpath}x" != "x" ] && {
			echo "Found path $devpath"
			for h in $hwaddr; do
				if [ "$(cat ${h})" = "00:00:11:12:13:14" ]; then
					ifname=$(echo ${h} | awk -F [\/] '{print $(NF-1)}')
				fi
			done
		} || {
			echo "Device path not found!"
			proto_notify_error "$interface" NO_DEVICE_FOUND
			return 1
		}
	}

	[ -n "$ifname" ] && {
		echo "Found interface $ifname"
	} || {
		echo "The interface could not be found."
		proto_notify_error "$interface" NO_IFACE
		proto_set_available "$interface" 0
		return 1
	}

	# probes for AT port and SIM-card
	for p in $(seq 1 $maxfail); do
		DEVPORT=$device gcom -s /etc/gcom/probeport.gcom
		DEVERR=$?
		[ "$DEVERR" = "0" ] && break
		if [ "$p" -eq "$maxfail" ]; then
			case $DEVERR in
				1)
					echo "AT port not answer!"
					proto_notify_error "$interface" NO_PORT_ANSWER
					proto_set_available "$interface" 0
					return 1
				;;
				2)
					echo "SIM-card not insert!"
					proto_notify_error "$interface" NO_SIM_CARD
					proto_set_available "$interface" 0
					return 1
				;;
			esac
		fi
	
		sleep 3
	done

	if [ -n "$pincode" ]; then
		PINCODE="$pincode" gcom -d "$device" -s /etc/gcom/setpin.gcom || {
			proto_notify_error "$interface" PIN_FAILED
			proto_block_restart "$interface"
			return 1
		}
	fi

	pdp=$(echo $pdp | awk '{print toupper($0)}')
	[ "$pdp" = "IP" -o "$pdp" = "IPV6" -o "$pdp" = "IPV4V6" ] || pdp="IP"
	echo "Setting up $ifname"
	[ -n "$username" ] && [ -n "$password" ] && {
		echo "Using auth type is: $auth"
		case $auth in
			pap) AUTH=1 ;;
			chap) AUTH=2 ;;
			*) AUTH=0 ;;
		esac
		CID=$profile AUTH=$AUTH USER="$username" PASS="$password" gcom -d "$device" -s /etc/gcom/${PREFIX}-auth.gcom >/dev/null 2>&1
	}

	CID=$profile APN=$apn PDP=$pdp  gcom -d $device -s /etc/gcom/${PREFIX}-connect.gcom >/dev/null 2>&1
	proto_init_update "$ifname" 1
	proto_add_data
	proto_close_data
	DATA=$(CID=$profile gcom -d $device -s /etc/gcom/${PREFIX}-config.gcom)
	ip4addr=$(echo "$DATA" | awk -F [,] '/^\+CGPADDR/{gsub("\r|\"", ""); print $2}') >/dev/null 2>&1
	lladdr=$(echo "$DATA" | awk -F [,] '/^\+CGPADDR/{gsub("\r|\"", ""); print $3}') >/dev/null 2>&1
	ns=$(echo "$DATA" | awk -F [,] '/^\+'$XMMDNS': /{gsub("\r|\"",""); gsub("0.0.0.0",""); print $2" "$3}' | sed 's/^[[:space:]]//g' | uniq)

	case $ip4addr in
		*FE80*)
			lladdr=$ip4addr
			ip4addr=""
		;;
		*)
			ip4mask=24
			defroute=$(echo $ip4addr | awk -F [.] '{print $1"."$2"."$3".1"}')
		;;
	esac

	for n in $(echo $ns); do
		$(valid_ip4 $n) && {
			[ ! "$(echo $dns1 | grep $n)" ] && {
				dns1="$dns1 $n"
			}
		}
	done

	proto_set_keep 1
	ip link set dev $ifname arp off
	echo "PDP type is: $pdp"
	[ "$pdp" = "IP" -o "$pdp" = "IPV4V6" ] && {
		$(valid_ip4 $ip4addr) && [ "$ip4addr" != "0.0.0.0" ] && {
			echo "Set IPv4 address: ${ip4addr}/${ip4mask}"
			proto_add_ipv4_address $ip4addr $ip4mask
			proto_add_ipv4_route "0.0.0.0" 0 $defroute $ip4addr
		} || {
			echo "Failed to configure interface"
			proto_notify_error "$interface" CONFIGURE_FAILED
			return 1
		}
		[ -n "$dns1" ] && {
			proto_add_dns_server "$dns1"
			echo "Using IPv4 DNS: $dns1"
		}
	}

	proto_add_data
	proto_close_data
	proto_send_update "$interface"

	[ "$pdp" = "IPV6" -o "$pdp" = "IPV4V6" ] && {
		json_init
		json_add_string name "${interface}_6"
		json_add_string ifname "@$interface"
		json_add_string proto "dhcpv6"
		json_add_string extendprefix 1
		proto_add_dynamic_defaults
		json_close_object
		ubus call network add_dynamic "$(json_dump)"
	}
}

proto_xmm_teardown() {
	local interface="$1"
	local device profile
	json_get_vars device profile
	[ "$profile" = "" ] && profile="1"
	CID=$profile gcom -d $device -s /etc/gcom/xmm-disconnect.gcom >/dev/null 2>&1
	echo "Modem $device disconnected"
	proto_kill_command "$interface"
}

[ -n "$INCLUDE_ONLY" ] || add_protocol xmm



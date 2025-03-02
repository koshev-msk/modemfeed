#!/bin/sh
. /lib/functions.sh
. /lib/netifd/netifd-proto.sh


find_xmm_iface() {
	local cfg="$1"
	local proto section

	config_get proto "$cfg" proto
	[ "$proto" = xmm ] || return 0

	local dev=$(uci_get network "$cfg" device)
	local devname=$(basename $dev)

	if ! [ -e "/sys/${DEVPATH}" ]; then
		if ! [ -e "/sys/class/tty/${devname}" ]; then
			if [ "$ACTION" = remove ]; then
				proto_set_available "$cfg" 0
			fi
		fi
	fi
	if [ -e "/sys/${DEVPATH}" ]; then
		if [ -e "/sys/class/tty/${devname}" ]; then
			if [ "$ACTION" = add ]; then
				proto_set_available "$cfg" 1
			fi
		fi
	fi
}

[ "$ACTION" = add ] || [ "$ACTION" = remove ] || exit 0

config_load network
config_foreach find_xmm_iface interface

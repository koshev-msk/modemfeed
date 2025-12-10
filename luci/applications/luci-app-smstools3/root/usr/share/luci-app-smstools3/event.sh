#!/bin/sh

killall smsd

. /lib/functions.sh

# Modems in system config
MODEMS=""
config_load smstools3

get_modem_names() {
	local modem_name="$1"
	config_get ENABLE "$modem_name" enable
	[ "$ENABLE" = "1" ] && MODEMS="${MODEMS}${MODEMS:+, }$modem_name"
}

config_foreach get_modem_names modem

# General settings using config_get
DECODE=$(uci -q get smstools3.@sms[0].decode_utf)
STORAGE=$(uci -q get smstools3.@sms[0].storage)
LOG=$(uci -q get smstools3.@sms[0].loglevel)
LED_EN=$(uci -q get smstools3.@sms[0].led_enable)

# Set default loglevel if not set
[ -z "$LOG" ] && LOG="5"


if [ ! -d /root/sms ]; then
	mkdir -p /root/sms
	for d in checked failed incoming outgoing sent; do
		mkdir -p /root/sms/${d}
	done
fi

case "$STORAGE" in
	persistent)
		if [ -d /var/spool/sms ]; then
			mv /var/spool/sms /var/spool/sms_tmp
			ln -sf /root/sms /var/spool/sms
		fi
	;;
	temporary)
		if [ -d /var/spool/sms_tmp ]; then
			rm -f /var/spool/sms
			mv /var/spool/sms_tmp /var/spool/sms
		fi
	;;
esac

# template config
echo "devices = $MODEMS"
echo "incoming = /var/spool/sms/incoming"
echo "outgoing = /var/spool/sms/outgoing"
echo "checked = /var/spool/sms/checked"
echo "failed = /var/spool/sms/failed"
echo "sent = /var/spool/sms/sent"
echo "receive_before_send = no"
echo "date_filename = 1"
echo "date_filename_format = %s"
echo "eventhandler = /usr/share/luci-app-smstools3/led.sh"

[ -n "$DECODE" ] && {
	echo "decode_unicode_text = yes"
	echo "incoming_utf8 = yes"
}

echo "receive_before_send = no"
echo "autosplit = 4"

[ -n "$LOG" ] && echo "loglevel = $LOG"

# Process each modem using config_foreach
process_modem() {
	local modem_name="$1"
	local UI DEVICE PIN INIT_ NET_CHECK SIG_CHECK ENABLE

	config_get UI "$modem_name" ui
	config_get DEVICE "$modem_name" device
	config_get PIN "$modem_name" pin
	config_get INIT_ "$modem_name" init
	config_get NET_CHECK "$modem_name" net_check
	config_get SIG_CHECK "$modem_name" sig_check
	config_get ENABLE "$modem_name" enable

	[ "$ENABLE" = "1" ] || return 0

	echo ""
	echo "[${modem_name}]"

	case "$INIT_" in
		huawei) echo "init = AT+CPMS=\"SM\";+CNMI=2,0,0,2,1" ;;
		intel) echo "init = AT+CPMS=\"SM\"" ;;
		asr) echo "init = AT+CPMS=\"SM\",\"SM\",\"SM\"" ;;
		*) echo "init = AT+CPMS=\"ME\",\"ME\",\"ME\"" ;;
	esac

	echo "device = $DEVICE"

	case "$SIG_CHECK" in
		1) echo "signal_quality_ber_ignore = yes" ;;
	esac

	case "$NET_CHECK" in
		0) echo "check_network = 0" ;;
		1) echo "check_network = 1" ;;
		2) echo "check_network = 2" ;;
	esac

	[ -z "$UI" ] && echo "detect_unexpected_input = no"

	echo "incoming = yes"

	# PIN validation
	[ -n "$PIN" ] && {
		case "${PIN#}" in
			*[!0-9]*)
				logger -t luci-app-smstools3 "invalid pin for modem $modem_name"
			;;
			*[0-9]*)
				[ ${#PIN} -lt 4 -a ${#PIN} -gt 8 ] && {
					echo "pin = $PIN"
				} || {
					logger -t luci-app-smstools3 "invalid pin length for modem $modem_name"
				}
			;;
		esac
	}

	echo "baudrate = 115200"
}

# Process all modems
config_foreach process_modem modem

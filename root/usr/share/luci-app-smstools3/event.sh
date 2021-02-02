#!/bin/sh

killall smsd

DECODE=$(uci -q get smstools3.@sms[0].decode_utf)
UI=$(uci -q get smstools3.@sms[0].ui)
STORAGE=$(uci -q get smstools3.@sms[0].storage)
DEVICE=$(uci -q get smstools3.@sms[0].device)
LOG=$(uci -q get smstools3.@sms[0].loglevel)
PIN=$(uci -q get smstools3.@sms[0].pin)
LED_EN=$(uci -q get smstools3.@sms[0].led_enable)

case $STORAGE in
	persistent)
		if [ -d  /var/spool/sms ]; then
			mv /var/spool/sms /var/spool/sms_tmp
			ln -s /root/sms /var/spool/sms
		fi
		;;
	temporary)
		if [ -d  /var/spool/sms_tmp ]; then
			rm -f /var/spool/sms
			mv /var/spool/sms_tmp /var/spool/sms
		fi
		;;
esac

# template config
echo -e "devices = GSM1\nincoming = /var/spool/sms/incoming\noutgoing = /var/spool/sms/outgoing"
echo -e "checked = /var/spool/sms/checked\nfailed = /var/spool/sms/failed\nsent = /var/spool/sms/sent"
echo -e "receive_before_send = no"

if [ $LED_EN ]; then
	echo "eventhandler = /usr/share/luci-app-smstools3/led.sh"
fi

if [ "$DECODE" ]; then
        echo "decode_unicode_text = yes"
        echo "incoming_utf8 = yes"
fi
echo -e "receive_before_send = no\nautosplit = 3"
if [ "$LOG" ]; then
	echo "loglevel = $LOG"
fi
echo ""
echo "[GSM1]"
echo "init = AT+CPMS=\"ME\",\"ME\",\"ME\""
echo "device = $DEVICE"
if [ ! "$UI" ]; then
        echo -e "detect_unexpected_input = no"
fi
echo "incoming = yes"
echo "baudrate = 115200"

#!/bin/sh

LED_EN=$(uci -q get smstools3.@sms[0].led_enable)
LED=$(uci -q get smstools3.@sms[0].led)

case $1 in
	off) 
		if [ $LED_EN ]; then
			echo none > /sys/class/leds/${LED}/trigger
		fi
	;;
	RECEIVED) 
		if [ $LED_EN ]; then
			echo timer > /sys/class/leds/${LED}/trigger 
		fi
	;;
esac

if [ -r /etc/smstools3.user ]; then
	. /etc/smstools3.user
fi

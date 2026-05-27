#!/bin/sh

# SIM slot switcher for HuastLink HC-G60 or ZBT-WE2806-A
# or similar CPE OpenWrt-routers
# by Konstantine Shevlakov at <shevlakov@132lan.ru> 2025
# Audited & fixed 2026

NODE="SW SIM"

# config file stored in /etc/config/ssw
# define gpios in /etc/config/system
#
# mandatory section names of gpio: modem and sim
# example config file /etc/config/system for gpio_switch
#
# config gpio_switch 'modem'
#        option name 'Modem pwr'
#        option gpio_pin '512'
#        option value '1'
#
# config gpio_switch 'sim'
#        option name 'SIM switch'
#        option gpio_pin 'sim'
#        option value '1'
#
# if gpio not defined on dts and have integer number on gpioswitch then
# option gpio must have value 'gpioN'. Example /etc/config/ssw:
#
# config modem 'modem'
#        option gpio 'gpio512'
#        option value '1'
#
# config sim 'sim'
#        option gpio 'sim'
#        option value '1'


. /lib/functions.sh

# Callback failover section
_load_failover(){
	config_get enable    "$1" enable    ""
	config_get interval  "$1" interval  60
	config_get revert    "$1" revert    ""
	config_get rsrp      "$1" rsrp      ""
	config_get times_rsrp "$1" times_rsrp 5
	config_get apn1      "$1" apn1      ""
	config_get apn2      "$1" apn2      ""
}

# Callback sections modem / sim (gpio type)
_load_gpio(){
	local _gpio _value
	config_get _gpio  "$1" gpio  ""
	config_get _value "$1" value ""
	eval "${1}_gpio=\"$_gpio\""
	eval "${1}_value=\"$_value\""
}

# Get Variables — update for every cycle
get_vars(){
	reset_cb
	config_load ssw
	config_foreach _load_failover failover
	config_foreach _load_gpio     modem
	config_foreach _load_gpio     sim
}

# SIM Switch
sw_sim(){
	case "$cur_sim" in
		0) next_sim=1 ;;
		1) next_sim=0 ;;
		*)
			logger -t "$NODE" "ERROR: cur_sim has unexpected value '$cur_sim'. Aborting sw_sim."
			return 1
			;;
	esac

	echo "0" > /sys/class/gpio/"$modem_gpio"/value
	echo "$next_sim" > /sys/class/gpio/"$sim_gpio"/value

	if [ "$modem_value" = "1" ]; then
		sleep 2
		echo "1" > /sys/class/gpio/"$modem_gpio"/value
		if [ -x /etc/init.d/smstools3 ]; then
			/etc/init.d/smstools3 restart
		fi
	fi
}

# Revert rule switch
sw_rule(){
	if [ -f /tmp/ssw.vars ]; then
		. /tmp/ssw.vars
	fi
	# Revert SIM card to default slot
	if [ "$cur_sim" -ne "$sim_value" ]; then
		if [ "$(date +%s)" -gt "$SWDATE" ]; then
			logger -t "$NODE" "Revert to default SIM slot with $apn"
			sw_sim
		fi
	fi
}

# Check interface state via mwan3
monitor_mwan3(){
	iface=$(uci show network | awk -F'[.]' '/devices/{print $2}' | tail -1)
	# Check link status
	if [ -r "/tmp/run/mwan3/iface_state/$iface" ]; then
		link_status=$(grep -c online "/tmp/run/mwan3/iface_state/$iface" || echo 0)
	else
		# Disable track link via mwan3
		link_status=1
	fi
}

check_signal(){
	local gen="$1" signal_json="$2"
	local metric value

	case "$gen" in
		lte|5g) metric="rsrp" ;;
		3g)     metric="rscp" ;;
		*)
			logger -t "$NODE" "check_signal: unknown generation '$gen'"
			return 1
			;;
	esac

	value=$(echo "$signal_json" \
		| jsonfilter -e "@[\"modem\"][*][\"${gen}\"][\"${metric}\"]" 2>/dev/null \
		| awk '{printf "%.0f\n", $1}')

	[ -n "$value" ] && echo "$value"
}

# RSRP/RSCP average value by modemmanager. Enable signal monitor!
monitor_rsrp(){
	local device SIGNAL CRSRP

	device="$(uci show network | awk -F'[=]' '/devices/{gsub("'\''",""); print $2}' | tail -1)"
	SIGNAL="$(mmcli -J -m "$device" --signal-get 2>/dev/null)"

	for gen in lte 5g 3g; do
		CRSRP=$(check_signal "$gen" "$SIGNAL")
		[ -n "$CRSRP" ] && break
	done

	if [ -n "$CRSRP" ] && [ "$CRSRP" -ne 0 ]; then
		echo "$CRSRP" >> /tmp/ssw_rsrp.var
	fi

	if [ "$cnt" -eq "$times_rsrp" ]; then
		if [ -s /tmp/ssw_rsrp.var ]; then
			RSRP=$(awk '{sum+=$1} END { printf "%.0f\n", sum/NR }' /tmp/ssw_rsrp.var)
		fi
		: > /tmp/ssw_rsrp.var

		if [ -n "$RSRP" ] && [ "$RSRP" -lt "$rsrp" ]; then
			mon_rsrp=0
		else
			mon_rsrp=1
		fi
	fi
}

# reload iface
reload_iface(){
	[ "$iface" ] && {
		for i in $iface; do
			ifup "$i"
		done
	}
}

cnt=0

while true; do
	get_vars
	sleep "$interval"

	if [ "$enable" = "1" ]; then
		cur_sim=$(cat /sys/class/gpio/"$sim_gpio"/value)

		cnt=$(( cnt + 1 ))
		monitor_rsrp
		monitor_mwan3

		if [ "$cnt" -eq "$times_rsrp" ]; then
			if [ "$link_status" = "0" ] || [ "$mon_rsrp" = "0" ]; then

				if [ "$sim_value" -eq "$cur_sim" ]; then
					apn=$apn2
				else
					apn=$apn1
				fi

				iface=$(uci show network | awk -F'[.]' '/devices/{gsub("'\''",""); print $2}' | tail -1)

				if [ -n "$RSRP" ]; then
					if [ "$mon_rsrp" = "0" ]; then
						logger -t "$NODE" "Modem interface: $iface average RSRP=${RSRP} dBm. Min. value ${rsrp} dBm."
					fi
					if [ "$link_status" = "0" ]; then
						logger -t "$NODE" "Modem interface: $iface lost connectivity."
					fi
				else
					logger -t "$NODE" "WARNING: RSRP value not available. Check 'Refresh signal' option of modem interface!"
				fi

				if [ "${#apn}" -gt 0 ]; then
					uci set network."$iface".apn="$apn"
					logger -t "$NODE" "Switch SIM-card slot with APN: $apn"
				else
					logger -t "$NODE" "WARNING: APN not defined. Switching SIM-card slot with default APN."
					uci set network."$iface".apn="internet"
				fi

				uci commit network
				reload_config

				if [ "$revert" = "1" ]; then
					if [ "$cur_sim" -eq "$sim_value" ]; then
						FBT=${FBT:=$(( (interval + times_rsrp) * 2 ))}
						FBT=$(( FBT * 2 ))
						SWDATE=$(( $(date +%s) + FBT ))
						printf 'FBT=%s\nSWDATE=%s\n' "$FBT" "$SWDATE" > /tmp/ssw.vars
						logger -t "$NODE" "Back to default SIM-slot after $(date -d @"${SWDATE}")"
					fi
				fi

				sw_sim && sleep 20 && reload_iface &

			elif [ "$revert" = "1" ]; then
				sw_rule && sleep 20 && reload_iface &
			fi

			cnt=1
		fi
	fi
done
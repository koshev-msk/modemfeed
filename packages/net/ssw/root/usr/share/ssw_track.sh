#!/bin/sh

# SIM slot switcher for HuastLink HC-G60 or ZBT-WE2806-A
# or similiar cpe OpenWrt-routers
# by Konstantine Shevlakov at <shevlakov@132lan.ru> 2025-2026

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

# Schedule switch section example /etc/config/ssw:
#
# config schedule 'schedule'
#        option enable '1'
#        option time_on 'HH:MM'      # time to switch to reserve SIM (24h format)
#        option duration '60'         # minutes until revert to default SIM (0 = no revert)
#        option apn 'internet'        # APN for reserve SIM during schedule switch
#        option period 'daily'        # daily | interval | weekly
#        option period_days '3'       # for period=interval: every N days
#        option weekday '1'           # for period=weekly: 0=Sun 1=Mon … 6=Sat


NODE="SW SIM"

# Get Variables
get_vars(){
	for v in enable interval revert rsrp times_rsrp apn1 apn2; do
		eval $v=$(uci -q get ssw.failover.${v} 2>/dev/null)
	done
	for d in modem sim; do
		for s in gpio value; do
			eval "${d}_${s}=$(uci -q get ssw.${d}.${s} 2>/dev/null)"
		done
	done
	[ -n "$interval" ] || interval=60
	[ -n "$times_rsrp" ] || times_rsrp=5

	# Schedule variables
	sched_enable=$(uci -q get ssw.schedule.enable 2>/dev/null)
	sched_time_on=$(uci -q get ssw.schedule.time_on 2>/dev/null)
	sched_duration=$(uci -q get ssw.schedule.duration 2>/dev/null)
	sched_apn=$(uci -q get ssw.schedule.apn 2>/dev/null)
	sched_period=$(uci -q get ssw.schedule.period 2>/dev/null)
	sched_period_days=$(uci -q get ssw.schedule.period_days 2>/dev/null)
	sched_weekday=$(uci -q get ssw.schedule.weekday 2>/dev/null)
	[ -n "$sched_duration" ]    || sched_duration=0
	[ -n "$sched_period" ]      || sched_period=daily
	[ -n "$sched_period_days" ] || sched_period_days=1
	[ -n "$sched_weekday" ]     || sched_weekday=1
}

# SIM Switch
sw_sim(){
	case $cur_sim in
		0) next_sim=1 ;;
		1) next_sim=0 ;;
	esac

	echo "0" > /sys/class/gpio/$modem_gpio/value
	echo "$next_sim" > /sys/class/gpio/$sim_gpio/value

	if [ "$modem_value" = "1" ]; then
		sleep 2
		echo "1" > /sys/class/gpio/$modem_gpio/value
		if [ -x /etc/init.d/smstools3 ]; then
			/etc/init.d/smstools3 restart
		fi
	fi
	set > /tmp/apn.ssw
}

# Revert rule switch (failover)
sw_rule(){
	if [ -f /tmp/ssw.vars ]; then
		. /tmp/ssw.vars
	fi
	if [ "$cur_sim" -ne "$sim_value" ]; then
		if [ "$(date +%s)" -gt "$SWDATE" ]; then
			logger -t "$NODE" "Revert to default SIM slot with $apn"
			sw_sim
		fi
	fi
}

# Schedule: revert to default SIM after duration
sched_revert(){
	[ -f /tmp/ssw_sched.vars ] && . /tmp/ssw_sched.vars

	if [ -n "$SCHED_REVERT_AT" ] && [ "$(date +%s)" -ge "$SCHED_REVERT_AT" ]; then
		cur_sim=$(cat /sys/class/gpio/$sim_gpio/value)
		if [ "$cur_sim" -ne "$sim_value" ]; then
			logger -t "$NODE" "Schedule: reverting to default SIM slot"
			iface=$(uci show network | awk -F [.] '/devices/{gsub("'\''","");print $2}' | tail -1)
			uci set network.$iface.apn="${apn1:-internet}"
			uci commit network
			reload_config network
			sw_sim && sleep 20 && reload_iface &
		fi
		rm -f /tmp/ssw_sched.vars
		logger -t "$NODE" "Schedule: revert done, schedule state cleared"
	fi
}

# Check if today matches the configured schedule period.
# Returns 0 (true) if the switch should fire today, 1 otherwise.
sched_day_match(){
	today=$(date +%Y%m%d)
	now_dow=$(date +%w)   # 0=Sun .. 6=Sat

	case "$sched_period" in
		daily)
			return 0
			;;

		weekly)
			[ "$now_dow" = "$sched_weekday" ] && return 0
			return 1
			;;

		interval)
			# Read the date of the last successful switch from persistent state.
			# We keep it in /tmp/ssw_sched_last.date so it survives across
			# the daemon's iterations but resets on reboot (intentional —
			# after reboot the interval counter restarts from today).
			last_date=""
			[ -f /tmp/ssw_sched_last.date ] && last_date=$(cat /tmp/ssw_sched_last.date)

			if [ -z "$last_date" ]; then
				# First run ever (or after reboot): trigger today.
				return 0
			fi

			# Calculate elapsed days between last_date and today (YYYYMMDD arithmetic).
			# Use date to convert both to unix timestamps then divide.
			ts_last=$(date -d "$last_date" +%s 2>/dev/null)
			[ -z "$ts_last" ] && ts_last=$(date -jf "%Y%m%d" "$last_date" +%s 2>/dev/null)
			ts_now=$(date +%s)
			elapsed_days=$(( (ts_now - ts_last) / 86400 ))

			[ "$elapsed_days" -ge "$sched_period_days" ] && return 0
			return 1
			;;

		*)
			# Unknown period — treat as daily
			return 0
			;;
	esac
}

# Schedule: switch to reserve SIM at configured time
check_schedule(){
	[ "$sched_enable" = "1" ] || return
	[ -n "$sched_time_on" ]   || return

	# Handle pending revert first — takes priority over new switch logic.
	if [ -f /tmp/ssw_sched.vars ]; then
		sched_revert
		return
	fi

	# Parse HH:MM (strip leading zeros to avoid octal)
	sched_h=$(echo "$sched_time_on" | cut -d: -f1 | sed 's/^0*//')
	sched_m=$(echo "$sched_time_on" | cut -d: -f2 | sed 's/^0*//')
	[ -z "$sched_h" ] && sched_h=0
	[ -z "$sched_m" ] && sched_m=0

	now_h=$(date +%H | sed 's/^0*//')
	now_m=$(date +%M | sed 's/^0*//')
	[ -z "$now_h" ] && now_h=0
	[ -z "$now_m" ] && now_m=0

	# Check time window: [sched_time .. sched_time + ceil(interval/60)) minutes
	now_total=$(( now_h * 60 + now_m ))
	sched_total=$(( sched_h * 60 + sched_m ))
	interval_min=$(( interval / 60 + 1 ))
	diff=$(( now_total - sched_total ))

	# Not in the time window yet
	if [ "$diff" -lt 0 ] || [ "$diff" -ge "$interval_min" ]; then
		return
	fi

	# Check if the period condition is met (daily / weekly / every N days)
	sched_day_match || return

	# Check if we already fired today (prevents re-triggering within the same window)
	today=$(date +%Y%m%d)
	if [ -f /tmp/ssw_sched_fired.date ]; then
		fired=$(cat /tmp/ssw_sched_fired.date)
		[ "$fired" = "$today" ] && return
	fi

	# All conditions met — switch to reserve SIM
	cur_sim=$(cat /sys/class/gpio/$sim_gpio/value)
	if [ "$cur_sim" -ne "$sim_value" ]; then
		# Already on reserve SIM (e.g. failover active), skip but record the day.
		logger -t "$NODE" "Schedule: already on reserve SIM, skipping switch"
		echo "$today" > /tmp/ssw_sched_fired.date
		return
	fi

	logger -t "$NODE" "Schedule: switching to reserve SIM (period=$sched_period, time=$sched_time_on)"
	iface=$(uci show network | awk -F [.] '/devices/{gsub("'\''","");print $2}' | tail -1)
	apn_use="${sched_apn:-${apn2:-internet}}"
	uci set network.$iface.apn="$apn_use"
	uci commit network
	reload_config network
	sw_sim && sleep 20 && reload_iface &

	# Record fire date (suppress re-trigger within same day/window)
	echo "$today" > /tmp/ssw_sched_fired.date

	# For interval mode: record as the last successful switch date
	if [ "$sched_period" = "interval" ]; then
		echo "$today" > /tmp/ssw_sched_last.date
	fi

	# Arm the revert timer if duration > 0
	if [ "$sched_duration" -gt 0 ]; then
		revert_at=$(( $(date +%s) + sched_duration * 60 ))
		echo "SCHED_REVERT_AT=$revert_at" > /tmp/ssw_sched.vars
		logger -t "$NODE" "Schedule: will revert to default SIM in ${sched_duration} min"
	else
		logger -t "$NODE" "Schedule: no auto-revert configured"
	fi
}

# Check interface state via mwan3
monitor_mwan3(){
	iface=$(uci show network | awk -F [.] '/devices/{print $2}')
	if [ -r /tmp/run/mwan3/iface_state/$iface ]; then
		link_status=$(cat /tmp/run/mwan3/iface_state/$iface | grep online | wc -l)
	else
		link_status=1
	fi
}

# RSRP average value by modemmanager. Enable signal monitor!
monitor_rsrp(){
	device="$(uci show network | awk -F [=] '/devices/{gsub("'\''","");print $2}')"
	SIGNAL="$(mmcli -J -m $device --signal-get)"
	CRSRP=$(echo "$SIGNAL" | jsonfilter -e '@["modem"][*]["lte"]["rsrp"]' | awk '{printf "%.0f\n", $1}')
	if ! [ $CRSRP ]; then
		CRSRP=$(echo "$SIGNAL" | jsonfilter -e '@["modem"][*]["5g"]["rsrp"]' | awk '{printf "%.0f\n", $1}')
	fi

	if [ $CRSRP -ne 0 ]; then
		echo $CRSRP >> /tmp/ssw_rsrp.var
	fi
	if [ $cnt -eq $times_rsrp ]; then
		RSRP=$(awk '{sum+=$1} END { printf "%.0f\n", sum/NR }' /tmp/ssw_rsrp.var)
		cat /dev/null > /tmp/ssw_rsrp.var
		if [ $RSRP -lt $rsrp ]; then
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
			ifup $i
		done
	}
}

# Stuff
cnt=1
while true; do
	get_vars
	sleep $interval

	# Schedule check runs every iteration, independently of failover
	check_schedule

	if [ "$enable" = "1" ]; then
		cur_sim=$(cat /sys/class/gpio/$sim_gpio/value)
		monitor_rsrp
		monitor_mwan3
		if [ "$cnt" -eq "$times_rsrp" ]; then
			if [ "$link_status" = "0" -o "$mon_rsrp" = "0" ]; then

				if [ "$sim_value" -eq "$cur_sim" ]; then
					apn=$apn2
				else
					apn=$apn1
				fi
				iface=$(uci show network | awk -F [.] '/devices/{gsub("'\''","");print $2}' | tail -1)
				if [ $RSRP ]; then
					if [ "$mon_rsrp" = "0" ]; then
						logger -t "$NODE" "Modem interface: $iface is average RSRP= ${RSRP} dBm. Min. value ${rsrp} dBm."
					fi
					if [ "$link_status" = "0" ]; then
						logger -t "$NODE" "Modem interface: $iface is loss connectivity"
					fi
				else
					logger -t "$NODE" "WARNING: RSRP value not exist. Please check \"Resfesh signal\" option of modem interface!"
				fi
				if [ "${#apn}" -gt "0" ]; then
					uci set network.$iface.apn="$apn"
					logger -t "$NODE" "Switch SIM-card slot with APN: $apn"
				else
					logger -t "$NODE" "WARNING: APN not defined. Switch SIM-card slot with default APN."
					uci set network.$iface.apn="internet"
				fi

				uci commit network
				reload_config network

				if [ "$revert" = "1" ]; then
					if [ "$cur_sim" -eq "$sim_value" ]; then
						FBT=${FBT:=$((($interval+$times_rsrp)*2))}
						FBT=$(($FBT*2))
						SWDATE=$((`date +%s`+$FBT))
						echo "FBT=$FBT" > /tmp/ssw.vars
						echo SWDATE=$SWDATE >> /tmp/ssw.vars
						logger -t "$NODE" "Back to default SIM-slot after $(date -d @${SWDATE})"
					fi
				fi
				sw_sim && sleep 20 && reload_iface &
			fi
			if [ "$revert" = "1" ]; then
				sw_rule && sleep 20 && reload_iface &
			fi
			cnt=0
		else
			cnt=$(($cnt+1))
		fi
	fi
done

#!/bin/sh

# SIM slot switcher for HuastLink HC-G60 or ZBT-WE2806-A
# or similiar cpe OpenWrt-routers
# by Konstantine Shevlakov at <shevlakov@132lan.ru> 2025

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


# Get Variables
get_vars(){
	for v in enable interval revert rsrp times_rsrp apn1 apn2; do
		eval $v=$(uci -q get ssw.failover.${v} 2>/dev/nul)
	done
	for d in modem sim; do
		for s in gpio value; do
			eval "${d}_${s}=$(uci -q get ssw.${d}.${s} 2>/dev/null)"
		done 
	done
	[ -n "$interval" ] || interval=60
	[ -n "$times_rsrp" ] || times_rsrp=5
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
	iface=$(uci show network | awk -F [.] '/devices/{print $2}')
	# Check link status
	if [ -r /tmp/run/mwan3/iface_state/$iface ]; then
		link_status=$(cat /tmp/run/mwan3/iface_state/$iface | grep online | wc -l)
	else
		# Disable track link via mwan3
		link_status=1
	fi
}

# RSRP average value by modemmanager. Enable singnal monitor!
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

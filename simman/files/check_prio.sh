#!/bin/sh

EPMTY_SIM=$(/etc/simman/getsimcheck.sh | grep "NOT INSERTED")
if [[ -n "$EPMTY_SIM" ]]; then
	echo "One or both SIM cards are not inserted"
	exit 1
fi

/etc/init.d/simman stop &>/dev/null

TESTIP0=$(uci -q get simman.core.testip && uci -q get simman.@sim0[0].testip)
TESTIP1=$(uci -q get simman.core.testip && uci -q get simman.@sim1[0].testip)
ATPORT=$(uci  -q get simman.core.atdevice)
IFNAME=$(uci -q get simman.core.iface)

LOG_PATH=$3
if [[ -z "$LOG_PATH" ]]; then
	LOG_PATH=/tmp/sim_prio_log
fi

PING0=0
PING1=0

ASU=0
ASU0=0
ASU1=0
NET0=2
NET1=2

ASU2G=$(echo $1 | grep "2G" | awk -F':' '{print $2}')
if [[ -z "$ASU2G" ]]; then
	ASU2G=0
fi
ASU3G=$(echo $2 | grep "3G" | awk -F':' '{print $2}')
if [[ -z "$ASU3G" ]]; then
	ASU3G=0
fi

#echo $ASU2G:$ASU3G

UP_COUNT=0
DOWN_COUNT=0

set_high(){
	if [[ "$1" -eq "0" ]]; then
		uci set simman.@sim0[0].priority='1'
		uci set simman.@sim1[0].priority='0'
		echo "$(date): set high SIM0" >> $LOG_PATH
	elif [[ "$1" -eq "1" ]]; then
		uci set simman.@sim0[0].priority='0'
		uci set simman.@sim1[0].priority='1'
		echo "$(date): set high SIM1" >> $LOG_PATH
	fi
}

/etc/simman/setsim.sh -s0
sleep 60

for IP in $TESTIP0
do
	COUNT=0
	while [[ "$COUNT" -le "2"  ]]; do
		ping -w5 -c1 -s8 -I $IFNAME $IP &> /dev/null
		if [[ "$?" -eq "0" ]]; then
			let UP_COUNT++
			PING0=1
		else
			let DOWN_COUNT++
		fi
		ASU=$(gcom -d $ATPORT -s /etc/simman/getsiglev.gcom | awk -F',' '{print $1}')
		if [[ "$ASU" -eq "99" ]]; then
			ASU=0
		fi
		if [[ "$ASU" -gt "$ASU0" ]]; then
			ASU0=$ASU
		fi
		#echo $ASU0
		sleep 2
		let COUNT++
	done
done

#echo "UP:$UP_COUNT;DOWN:$DOWN_COUNT"
#echo ASU0$ASU0

NET=$(/etc/simman/getnettype.sh)
if [[ "$NET" == "3G" ]]; then
	NET0=3
fi
#echo NET:$NET

UP_COUNT=0
DOWN_COUNT=0
ASU=0
NET=0

/etc/simman/setsim.sh -s1
sleep 60

for IP in $TESTIP1
do
	COUNT=0
	while [[ "$COUNT" -le "2"  ]]; do
		ping -w5 -c1 -s8 -I $IFNAME $IP &> /dev/null
		if [[ "$?" -eq "0" ]]; then
			let UP_COUNT++
			PING1=1
		else
			let DOWN_COUNT++
		fi
		ASU=$(gcom -d $ATPORT -s /etc/simman/getsiglev.gcom | awk -F',' '{print $1}')
		if [[ "$ASU" -eq "99" ]]; then
			ASU=0
		fi
		if [[ "$ASU" -gt "$ASU1" ]]; then
			ASU1=$ASU
		fi
		#echo $ASU1
		sleep 2
		let COUNT++
	done
done

#echo "UP:$UP_COUNT;DOWN:$DOWN_COUNT"
#echo ASU1$ASU1

NET=$(/etc/simman/getnettype.sh)
if [[ "$NET" == "3G" ]]; then
	NET1=3
fi
#echo NET:$NET

echo "$(date): SIM0: ping=$PING0 NET:$NET0 $ASU0 ASU; SIM1: ping=$PING1 NET:$NET1 $ASU1 ASU" >> $LOG_PATH

if [[ "$PING0" -eq "0" ]]; then
	if [[ "$PING1" -eq "0" ]]; then
		echo "Ping is not available on both SIM cards"
		if [[ "$NET0" -gt "$NET1" ]]; then
			set_high 0
		elif [[ "$NET0" -lt "$NET1" ]]; then
			set_high 1
		elif [[ "$ASU0" -gt "$ASU1" ]]; then
			set_high 0
		elif [[ "$ASU0" -lt "$ASU1" ]]; then
			set_high 1
		fi
	else
		set_high 1
	fi
else
	if [[ "$PING1" -eq "0" ]]; then
		set_high 0
	else
		if [[ "$NET0" -gt "$NET1" ]]; then
			if [[ "$ASU0" -ge "$ASU3G" ]]; then
				set_high 0
			elif [[ "$ASU1" -ge "$ASU2G" ]]; then
				set_high 1
			fi
		elif [[ "$NET0" -lt "$NET1" ]]; then
			if [[ "$ASU1" -ge "$ASU3G" ]]; then
				set_high 1
			elif [[ "$ASU0" -ge "$ASU2G" ]]; then
				set_high 0
			fi
		elif [[ "$ASU0" -gt "$ASU1" ]]; then
			set_high 0
		elif [[ "$ASU0" -lt "$ASU1" ]]; then
			set_high 1
		fi
	fi
fi

/etc/init.d/simman start &>/dev/null

exit 0

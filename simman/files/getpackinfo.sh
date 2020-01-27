#!/bin/sh

OPTIND=1

SCRIPT_PACKINFO="/etc/simman/getpackinfo.gcom"
SCRIPT_PACKINFO1="/etc/simman/getpackinfo1.gcom"
SCRIPT_PACKINFOLTE="/etc/simman/getpackinfolte.gcom"
device=""
PACKINFO=""
proto=""
counter=0

while getopts "h?d:" opt; do
	case "$opt" in
	h|\?)
	  echo "Usage: ./getpackinfo.sh [option]"
	  echo "Options:"
	  echo " -d - AT modem device"
	  echo "Example: getpackinfo.sh -d /dev/ttyACM3"
	  exit 0
	;;
	d) device=$OPTARG
	;;
	esac
done

shift $((OPTIND-1))
[ "$1" = "--" ] && shift

[ -z "$device" ] && device=$(uci -q get simman.core.atdevice)

# Check if device exists
[ ! -e $device ] && exit 0

PROTO_3G=$(uci get simman.core.proto 2>/dev/null)
if [ "$PROTO_3G" = "3" ];then
	echo "ALL:" > /tmp/lock/smsd.lock
fi

proto=$(uci -q get simman.core.proto)

if [ "$proto" = "0" ]; then
	PACKINFO=$(gcom -d $device -s $SCRIPT_PACKINFO | awk -F',' '{print $3}')
	[ -z "$PACKINFO" ] && PACKINFO="NONE"

	if [ "$PACKINFO" -eq 0 ]; then
		echo "'GPRS/EGPRS not available'"
	else 
		if [ "$PACKINFO" -eq 2 ]; then
			echo "'GPRS'"
		else 
			if [ "$PACKINFO" -eq 4 ]; then
				echo "'EGPRS'"
			else 
				if [ "$PACKINFO" -eq 6 ]; then
					echo "'WCDMA'"
				else
					if [ "$PACKINFO" -eq 8 ]; then
						echo "'HSDPA'"
					else
						if [ "$PACKINFO" -eq 10 ]; then
							echo "'HSDPA/HSUPA'"
						else
							echo "'UNKNOWN'"
						fi
					fi
				fi
			fi
		fi
	fi
elif [ "$proto" == "3" ]; then
	PACKINFO=$(gcom -d $device -s $SCRIPT_PACKINFO1 | awk -F',' '{print $1}')
	[ -z "$PACKINFO" ] && PACKINFO="NONE"
	case "$PACKINFO" in
				0)
			echo "'GSM'"
		;;
		1)
			echo "'GPRS'"
		;;
		2)
			echo "'WCDMA'"
		;;
		3)
			echo "'EGPRS (EDGE)'"
		;;
		4)
			echo "'HSDPA only(WCDMA)'"
		;;
		5)
			echo "'HSUPA only(WCDMA)'"
		;;
		6)
			echo "'HSPA (HSDPA and HSUPA, WCDMA)'"
		;;
		7)
			echo "'LTE'"
		;;
	esac
else

	PACKINFO=$(gcom -d $device -s $SCRIPT_PACKINFOLTE | awk -F',' '{print $2}')

	[ -z "$PACKINFO" ] && PACKINFO="NONE"
	case "$PACKINFO" in
		0)
			echo "'No service'"
		;;
		1)
			echo "'GSM'"
		;;
		2)
			echo "'GPRS'"
		;;
		3)
			echo "'EGPRS (EDGE)'"
		;;
		4)
			echo "'WCDMA'"
		;;
		5)
			echo "'HSDPA only(WCDMA)'"
		;;
		6)
			echo "'HSUPA only(WCDMA)'"
		;;
		7)
			echo "'HSPA (HSDPA and HSUPA, WCDMA)'"
		;;
		8)
			echo "'LTE'"
		;;
	esac
fi

if [ "$PROTO_3G" = "3" ];then
	rm /tmp/lock/smsd.lock
fi

exit 0
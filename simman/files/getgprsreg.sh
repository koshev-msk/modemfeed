#!/bin/sh

OPTIND=1

SCRIPT_GREG="/etc/simman/getgprsreg.gcom"
device=""
GREG=""

while getopts "h?d:" opt; do
	case "$opt" in
	h|\?)
	  echo "Usage: ./getgprsreg.sh [option]"
	  echo "Options:"
	  echo " -d - AT modem device"
	  echo "Example: getgprsreg.sh -d /dev/ttyACM3"
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


GREG=$(gcom -d $device -s $SCRIPT_GREG | grep -e [0-9] | awk -F',' '{print $2}')
[ -z "$GREG" ] && GREG="NONE"


if [ "$GREG" -eq 0 ]; then
	echo "'NOT REGISTERED'"
else 
	if [ "$GREG" -eq 1 ]; then
		echo "'REGISTERED, HOME'"
	else 
		if [ "$GREG" -eq 2 ]; then
			echo "'NOT REGISTERED, OPERATOR SEARCH'"
		else 
			if [ "$GREG" -eq 3 ]; then
				echo "'REGISTRATION DENIED'"
			else
				if [ "$GREG" -eq 5 ]; then
					echo "'REGISTERED, ROAMING!'"
				else
					echo "'UNKNOWN'"
				fi
			fi
		fi
	fi
fi

if [ "$PROTO_3G" = "3" ];then
	rm /tmp/lock/smsd.lock
fi

exit 0
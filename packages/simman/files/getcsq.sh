#!/bin/sh

OPTIND=1

SCRIPT_SIGLEV="/etc/simman/getsiglev.gcom"
device=""
SIGLEV=99
SIGQUAL=""

while getopts "h?d:" opt; do
	case "$opt" in
	h|\?)
	  echo "Usage: ./getsiglev.sh [option]"
	  echo "Options:"
	  echo " -d - AT modem device"
	  echo "Example: getsiglev.sh -d /dev/ttyACM3"
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


SIGLEV=$(gcom -d $device -s $SCRIPT_SIGLEV | grep -e [0-9] | awk -F',' '{print $1}')
[ -z "$SIGLEV" ] && SIGLEV="99"

echo "$SIGLEV"

if [ "$PROTO_3G" = "3" ];then
	rm /tmp/lock/smsd.lock
fi

exit 0
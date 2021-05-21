#!/bin/sh

OPTIND=1

SCRIPT_CFUN="/etc/simman/setfun.gcom"
device=""
IMEI=""
counter=0

while getopts "h?f:" opt; do
	case "$opt" in
	h|\?)
	  echo "Usage: ./setfun.sh [option]"
	  echo "Options:"
	  echo " -f - CFUN mode"
	  echo "Example: setfun.sh -f 0"
	  exit 0
	;;
	f) cfun=$OPTARG
	esac
done

shift $((OPTIND-1))
[ "$1" = "--" ] && shift

[ -z "$device" ] && device=$(uci -q get simman.core.atdevice)

PROTO_3G=$(uci get simman.core.proto 2>/dev/null)
if [ "$PROTO_3G" = "3" ];then
	echo "ALL:" > /tmp/lock/smsd.lock
fi

# Check if device exists
[ ! -e $device ] && exit 0
while [ "$RESULT" != "$cfun" ]; do
	RESULT=$(COMMAND="=$cfun" gcom -d $device -s $SCRIPT_CFUN)
	sleep 1 
	RESULT=$(COMMAND="?" gcom -d $device -s $SCRIPT_CFUN)
done

if [ "$PROTO_3G" = "3" ];then
	rm /tmp/lock/smsd.lock
fi

exit 0
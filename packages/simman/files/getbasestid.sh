#!/bin/sh

OPTIND=1

SCRIPT_BASESTINFO="/etc/simman/getbasestinfo.gcom"
SCRIPT_BASESTINFO1="/etc/simman/getbasestinfo1.gcom"
SCRIPT_BASEIDINFO="/etc/simman/getbaseidinfo.gcom"
device=""
BASESTINFO=""
NETTYPE=""
BASESTID=""
proto=""

while getopts "h?d:" opt; do
        case "$opt" in
        h|\?)
          echo "Usage: ./getbasestid.sh [option]"
          echo "Options:"
          echo " -d - AT modem device"
          echo "Example: getbasestid.sh -d /dev/ttyACM3"
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
  BASESTINFO=$(gcom -d $device -s $SCRIPT_BASESTINFO)
  [ -z "$BASESTINFO" ] && BASESTINFO="NONE"

  NETTYPE=${BASESTINFO:0:2}

  if [ "$NETTYPE" == "3G" ]
  then
        BASESTID=$( echo $BASESTINFO | awk -F',' '{print $9}')
        [ -z "$BASESTID" ] && BASESTID="SEARCH"
        echo $BASESTID
  else
        if [ "$NETTYPE" == "2G" ]
        then
                BASESTID=$( echo $BASESTINFO | awk -F',' '{print $7}')
                [ -z "$BASESTID" ] && BASESTID="SEARCH"
                echo $BASESTID
        else
                echo "Identification failed"
        fi
  fi
elif [ "$proto" = "3" ]; then
  BASESTINFO=$(gcom -d $device -s $SCRIPT_BASESTINFO1)
  BASESTID=$( echo $BASESTINFO | awk -F',' '{print $4}')
  echo $BASESTID
else
  BASESTINFO=$(gcom -d $device -s $SCRIPT_BASEIDINFO)
  [ -z "$BASESTINFO" ] && BASESTINFO="NONE"
  
  BASESTID=$( echo $BASESTINFO | awk -F',' '{print $5}')
  BASESTID=$(echo "obase=16; $BASESTID" | bc)
  [ -z "$BASESTID" ] && BASESTID="SEARCH"
  echo $BASESTID
fi

if [ "$PROTO_3G" = "3" ];then
  rm /tmp/lock/smsd.lock
fi

exit 0
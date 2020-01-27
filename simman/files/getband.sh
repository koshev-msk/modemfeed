#!/bin/sh

OPTIND=1

SCRIPT_BASESTINFO="/etc/simman/getbasestinfo.gcom"
SCRIPT_BASESTINFO1="/etc/simman/getenginfo.gcom"
SCRIPT_BASEIDINFO="/etc/simman/getbaseidinfo.gcom"
device=""
BASESTINFO=""
NETTYPE=""
BAND=""
proto=""
counter=0

while getopts "h?d:" opt; do
        case "$opt" in
        h|\?)
          echo "Usage: ./getband.sh [option]"
          echo "Options:"
          echo " -d - AT modem device"
          echo "Example: getband.sh -d /dev/ttyACM3"
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
        BAND=$( echo $BASESTINFO | awk -F',' '{print $2}')
        [ -z "$BAND" ] && BAND="Search..."
        echo "'UARFCN $BAND'"
  else
        if [ "$NETTYPE" == "2G" ]
        then
                BAND=$( echo $BASESTINFO | awk -F',' '{print $2}')
                [ -z "$BAND" ] && BAND="Search..."
                echo "'ARFCN $BAND'"
        else
                echo "Identification failed"
        fi
  fi
elif [ "$proto" == 3 ]; then
  BASESTINFO=$(gcom -d $device -s $SCRIPT_BASESTINFO1)
  [ -z "$BASESTINFO" ] && BASESTINFO="NONE"

  NETTYPE=$( echo $BASESTINFO | awk -F':' '{print $1}')

  if [ "$NETTYPE" == "UMTS" ]; then
        BAND=$( echo $BASESTINFO | awk -F'"' '{print $2}' | awk -F',' '{print $1}')
        [ -z "$BAND" ] && BAND="Search..."
        echo "'UARFCN $BAND'"   
  elif [ "$NETTYPE" == "GSM" ]; then
        BAND=$( echo $BASESTINFO | awk -F'"' '{print $2}' | awk -F',' '{print $1}')
        [ -z "$BAND" ] && BAND="Search..."
        echo "'ARFCN $BAND'"   
  fi

else
  BASESTINFO=$(gcom -d $device -s $SCRIPT_BASEIDINFO)  

  [ -z "$BASESTINFO" ] && BASESTINFO="NONE"
  NETTYPE=$( echo $BASESTINFO | awk -F',' '{print $1}')

    if [ "$NETTYPE" == "LTE" ]; then
      BAND=$( echo $BASESTINFO | awk -F',' '{print $8}')
      [ -z "$BAND" ] && BAND="Search..."
      echo "'EARFCN $BAND'"
    else
      if [ "$NETTYPE" == "WCDMA" ]; then
        BAND=$( echo $BASESTINFO | awk -F',' '{print $8}')
        [ -z "$BAND" ] && BAND="Search..."
        echo "'UARFCN $BAND'"    
      else
        if [ "$NETTYPE" == "GSM" ]; then
          BAND=$( echo $BASESTINFO | awk -F',' '{print $6}')
          [ -z "$BAND" ] && BAND="Search..."
          echo "'ARFCN $BAND'"   
        fi
      fi
    fi
fi

if [ "$PROTO_3G" = "3" ];then
  rm /tmp/lock/smsd.lock
fi



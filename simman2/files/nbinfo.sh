#!/bin/sh

DEVICE="/dev/ttyAMA0"

send_at(){
	IMEI="$(gcom -d $DEVICE -s /etc/simman/sendat.gcom)"
	if [[ "$?" != "0" ]]; then
		echo "Not available"
		exit 1
	fi
}

imei(){
	IMEI="$(gcom -d $DEVICE -s /etc/simman/getimeinb.gcom)"
	if [[ "$?" != "0" ]]; then
		echo "Not available"
		exit 1
	fi
	echo $IMEI
}

setfun(){
	FUN=$(COMMAND="?" gcom -d /dev/ttyAMA0 -s /etc/simman/setfunnb.gcom)
	if [[ "$FUN" == "0" ]]; then
		FUN=$(COMMAND="=1" gcom -d /dev/ttyAMA0 -s /etc/simman/setfunnb.gcom)
		sleep 1
	else 
		FUN=$(COMMAND="=0" gcom -d /dev/ttyAMA0 -s /etc/simman/setfunnb.gcom)
		FUN=$(COMMAND="=1" gcom -d /dev/ttyAMA0 -s /etc/simman/setfunnb.gcom)
		sleep 1
	fi
}

ccid(){
	CCID="$(gcom -d $DEVICE -s /etc/simman/getccidnb.gcom)"
	if [[ "$?" != "0" ]]; then
		echo "Not available"
		exit 1
	fi	
	echo $CCID
}

imsi(){
	CCID="$(gcom -d $DEVICE -s /etc/simman/getccidnb.gcom)"
	if [[ "$?" != "0" ]]; then
		echo "Not available"
		exit 1
	fi	
	echo $CCID
}

case "$1" in
	imei)
		eval send_at
		eval imei
		exit 0
	;;
	ccid)
		eval send_at
		eval setfun
		eval ccid
		exit 0
	;;
	imsi)
		eval send_at
		eval setfun
		eval imsi
		exit 0
	;;
	*)
		logger -st nbinfo "Command is not supported. Use the following commands: imei ccid imsi"
		exit 0
	;;
esac

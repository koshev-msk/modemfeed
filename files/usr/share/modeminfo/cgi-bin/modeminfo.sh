#!/bin/sh

#
# (c) 2010-2016 Cezary Jackiewicz <cezary@eko.one.pl>
# (c) 2020 modified by Konstantine Shevlyakov  <shevlakov@132lan.ru>


RES="/usr/share/modeminfo"
TPL=$RES/modem.json

if [ ! -e $TPL ]; then
        exit 0
fi

function json_status() {
	sed -e "s!{DEVICE}!$DEVICE!g; \
	s!{COPS_MCC}!$COPS_MCC!g; \
	s!{COPS_MNC}!$COPS_MNC!g; \
	s!{COPS}!$COPS!g; \
	s!{MODE}!$MODE!g; \
	s!{CSQ_PER}!$CSQ_PER!g; \
	s!{LAC}!$LAC!g; \
	s!{LAC_NUM}!$LAC_NUM!g; \
	s!{CID}!$CID!g; \
	s!{CID_NUM}!$CID_NUM!g;\
	s!{CSQ_RSSI}!$CSQ_RSSI!g; \
	s!{SINR}!$SINR!g; \
	s!{RSRP}!$RSRP!g; \
	s!{RSRQ}!$RSRQ!g; \
	s!{IMEI}!$IMEI!g; \
	s!{REGST}!$REGST!g; \
	s!{CSQ_COL}!$CSQ_COL!g; \
	s!{BAND}!$BAND!g; \
	s!{FUL}!$FUL!g; \
	s!{FDL}!$FDL!g; \
	s!{EARFCN}!$EARFCN!g;\
	s!{CNNAME}!$CNNAME!g;\
	s!{SNRNAME}!$SNRNAME!g" $TPL
}

function freq_band(){
	case $MODE in
        LTE)
	if [ $EARFCN -ge 0 ] && [ $EARFCN -le 599 ]; then
                BAND="B1 FDD"
                FDL_LOW=2110
                FUL_LOW=1920
                NOFFDL=0
	elif [ $EARFCN -ge 1200 ] && [ $EARFCN -le 1949 ]; then
                BAND="B3 FDD"
                FDL_LOW=1805
                FUL_LOW=1710
                NOFFDL=1200
	elif [ $EARFCN -ge 2750 ] && [ $EARFCN -le 3449 ]; then
                BAND="B7 FDD"
                FDL_LOW=2620
                FUL_LOW=2500
                NOFFDL=2750
	elif [ $EARFCN -ge 6150 ] && [ $EARFCN -le 6449 ]; then
                BAND="B20 FDD"
                FDL_LOW=791
                FUL_LOW=832
                NOFFDL=6150
	elif [ $EARFCN -ge 9870 ] && [ $EARFCN -le 9919 ]; then
                BAND="B31 FDD"
                FDL_LOW=452
                FUL_LOW=462
                NOFFDL=9870
	elif [ $EARFCN -ge 33750 ] && [ $EARFCN -le 38249 ]; then
                BAND="B38 TDD"
                FDL_LOW=2570
                FUL_LOW=2570
                NOFFDL=33750
	elif  [ $EARFCN -ge 38650 ] && [ $EARFCN -le 39649 ]; then
                BAND="B40 TDD"
                FDL_LOW=2300
                FUL_LOW=2300
                NOFFDL=38650
	fi
	if [ $FUL_LOW ] && [ $FDL_LOW ]; then
                FDL=$(($FDL_LOW + (($EARFCN - $NOFFDL)/10)))
                FUL=$(($FUL_LOW + (($EARFCN - $NOFFDL)/10)))
	else
                FDL="-"
                FUL="-"
	fi
        ;;
        *)
	if [ $EARFCN -ge 10562 ] && [ $EARFCN -le 10838 ]; then
                BAND="IMT2100"
                OFFSET=950
                FDL=$(($EARFCN/5))
                FUL=$((($EARFCN - $OFFSET)/5))
	elif [ $EARFCN -ge 2937 ] && [ $EARFCN -le 3088 ]; then
                BAND="UMTS900"
                FUL_LOW=925
                OFFSET=340
                FUL=$(($OFFSET + ($EARFCN/5)))
                FDL=$(($FUL - 45))
	elif [ $EARFCN -ge 955 ] && [ $EARFCN -le 1023 ]; then
                BAND="DCS900"
                FUL_LOW=890
                FUL=$(($FUL_LOW + ($EARFCN - 1024)/5))
                FDL=$(($FUL + 45))
	elif  [ $EARFCN -ge 512 ] && [ $EARFCN -le 885 ]; then
                BAND="DSC1800"
                FUL_LOW=1710
                FUL=$(($FUL_LOW + ($EARFCN - 512)/5))
                FDL=$(($FUL + 95))
	elif [ $EARFCN -ge 1 ] && [ $EARFCN -le 124 ]; then
                BAND="GSM900"
                FUL_LOW=890
                FUL=$(($FUL_LOW + ($EARFCN/5)))
                FDL=$(($FUL + 45))
	else
                FUL="-"
                FDL="-"
	fi
        ;;
	esac
}
getpath() {
	devname="$(basename "$1")"
	case "$devname" in
	'tty'*)
		devpath="$(readlink -f /sys/class/tty/$devname/device)"
		P=${devpath%/*/*}
		;;
	*)
		devpath="$(readlink -f /sys/class/usbmisc/$devname/device/)"
		P=${devpath%/*}
		;;
	esac
}

# search device
DEVICE=$(uci -q get modeminfo.@modeminfo[0].device)
# Deprecated
if echo "x$DEVICE" | grep -q "192.168."; then
	if grep -q "Vendor=1bbb" /sys/kernel/debug/usb/devices; then
		O=$($RES/scripts/alcatel_hilink.sh $DEVICE)
	fi
	if grep -q "Vendor=12d1" /sys/kernel/debug/usb/devices; then
		O=$($RES/scripts/huawei_hilink.sh $DEVICE)
	fi
	if grep -q "Vendor=19d2" /sys/kernel/debug/usb/devices; then
		O=$($RES/scripts/zte.sh $DEVICE)
	fi
	SEC=$(uci -q get modeminfo.@modeminfo[0].network)
	SEC=${SEC:-wan}
else
	if [ "x$DEVICE" = "x" ]; then
		devices=$(ls /dev/ttyUSB* /dev/cdc-wdm* /dev/ttyACM* /dev/ttyHS* 2>/dev/null | sort -r);
		for d in $devices; do
			DEVICE=$d gcom -s $RES/scripts/probeport.gcom > /dev/null 2>&1
			if [ $? = 0 ]; then
				uci set modeminfo.@modeminfo[0].device="$d"
				uci commit modeminfo
				break
			fi
		done
		DEVICE=$(uci -q get modeminfo.@modeminfo[0].device)
	fi

	if [ "x$DEVICE" = "x" ]; then
		echo $NOTDETECTED
		exit 0
	fi

	if [ ! -e $DEVICE ]; then
		DEVICE=$NODEVICE
		COPS_MCC="-"
		COPS_MNC="-"
		COPS="-"
		MODE="-"
		IMEI='-'
		exit 0
	fi

	# íàéòè ñîîòâåòñòâóþùèé ðàçäåë â êîíôèãóðàöèè - à èìåííî - èìÿ èíòåðôåéñà ìîäåìà, íî åñëè íå ñîçäàâàë èíòåðôåéñ, òî õðåí âàì òîâàðèù (äëÿ 3G ìîäåìîâ - òîëüêî SMS þçàþ, è âïèñûâàþ ðóêàìè wan)
	SEC=$(uci -q get modeminfo.@modeminfo[0].network)
	if [ -z "$SEC" ]; then
		getpath $DEVICE
		PORIG=$P
		for DEV in /sys/class/tty/* /sys/class/usbmisc/*; do
			getpath "/dev/"${DEV##/*/}
			if [ "x$PORIG" = "x$P" ]; then
				SEC=$(uci show network | grep "/dev/"${DEV##/*/} | cut -f2 -d.)
				[ -n "$SEC" ] && break
			fi
		done
	fi

#	[ "${DEVICE%%[0-9]}" = "/dev/ttyUSB" ] && stty -F $DEVICE -iexten -opost -icrnl

	# get pincode
	if [ ! -f /tmp/pincode_was_given ]; then
		# only first
		if [ ! -z $SEC ]; then
			PINCODE=$(uci -q get network.$SEC.pincode)
		fi
		if [ -z "$PINCODE" ]; then
			PINCODE=$(uci -q get modeminfo.@modeminfo[0].pincode)
		fi
		if [ ! -z $PINCODE ]; then
			PINCODE="$PINCODE" gcom -d "$DEVICE" -s /etc/gcom/setpin.gcom > /dev/null || {
				echo "$PINERROR"
				exit 0
			}
		fi
		touch /tmp/pincode_was_given
	fi 
	O=$(gcom -d $DEVICE -s $RES/scripts/3ginfo.gcom 2>/dev/null)
fi

# CSQ
CSQ=$(echo "$O" | awk -F[,\ ] '/^\+CSQ/ {print $2}')

[ "x$CSQ" = "x" ] && CSQ=-1
if [ $CSQ -ge 0 -a $CSQ -le 31 ]; then

	# for Gargoyle
	[ -e /tmp/strength.txt ] && echo "+CSQ: $CSQ,99" > /tmp/strength.txt

	CSQ_PER=$(($CSQ * 100/31))
	CSQ_COL="red"
	[ $CSQ -ge 10 ] && CSQ_COL="red"
	[ $CSQ -ge 15 ] && CSQ_COL="orange"
	[ $CSQ -ge 20 ] && CSQ_COL="green"
	CSQ_RSSI=$((2 * CSQ - 113))
else
	CSQ="-"
	CSQ_PER="0"
	CSQ_COL="black"
	CSQ_RSSI="-"
fi

# COPS
COPS_NUM=$(echo "$O" | awk -F[\"] '/^\+COPS: .,2/ {print $2}')
if [ "x$COPS_NUM" = "x" ]; then
	COPS_NUM="-"
	COPS_MCC="-"
	COPS_MNC="-"
else
	COPS_MCC=${COPS_NUM:0:3}
	COPS_MNC=${COPS_NUM:3:3}
	COPS=$(awk -F[\;] '/'$COPS_NUM'/ {print $2}' $RES/mccmnc.dat)
	[ "x$COPS" = "x" ] && COPS="-"
fi

# Option è ZTE modems
if [ "$COPS_NUM" = "-" ]; then
	COPS=$(echo "$O" | awk -F[\"] '/^\+COPS: .,0/ {print $2}')
	[ "x$COPS" = "x" ] && COPS="---"

	COPS_TMP=$(awk -F[\;] 'BEGIN {IGNORECASE = 1} /'"$COPS"'/ {print $2}' $RES/mccmnc.dat)
	if [ "x$COPS_TMP" = "x" ]; then
		COPS_NUM="-"
		COPS_MCC="-"
		COPS_MNC="-"
	else
		COPS="$COPS_TMP"
		COPS_NUM=$(awk -F[\;] 'BEGIN {IGNORECASE = 1} /'"$COPS"'/ {print $1}' $RES/mccmnc.dat)
		COPS_MCC=${COPS_NUM:0:3}
		COPS_MNC=${COPS_NUM:3:3}
	fi
fi

# network mode (LTE/UMTS/WCDMA èòä)
MODE="-"

# Huawei new items
TECH=$(echo "$O" | awk -F[,] '/^\^SYSINFOEX/ {print $9}' | sed 's/"//g')
if [ "x$TECH" != "x" ]; then
	MODE=$(echo "$TECH" | sed 's/-//g')
fi

# Huawei and older models
if [ "x$MODE" = "x-" ]; then
	TECH=$(echo "$O" | awk -F[,] '/^\^SYSINFO/ {print $7}')
	case $TECH in
		17*) MODE="HSPA+ (64QAM)";;
		18*) MODE="HSPA+ (MIMO)";;
		1*) MODE="GSM";;
		2*) MODE="GPRS";;
		3*) MODE="EDGE";;
		4*) MODE="UMTS";;
		5*) MODE="HSDPA";;
		6*) MODE="HSUPA";;
		7*) MODE="HSPA";;
		9*) MODE="HSPA+";;
		 *) MODE="-";;
	esac
fi

# ZTE
if [ "x$MODE" = "x-" ]; then
	TECH=$(echo "$O" | awk -F[,\ ] '/^\+ZPAS/ {print $2}' | sed 's/"//g')
	if [ "x$TECH" != "x" -a "x$TECH" != "xNo" ]; then
		MODE="$TECH"
	fi
fi

# OPTION
if [ "x$MODE" = "x-" ]; then
	TECH=$(echo "$O" | awk -F, '/^\+COPS: 0/ {print $4}')
	MODE="-"
	if [ "$TECH" = 0 ]; then
		TECH1=$(echo "$O" | awk '/^_OCTI/ {print $2}' | cut -f1 -d,)
		case $TECH1 in
			1*) MODE="GSM";;
			2*) MODE="GPRS";;
			3*) MODE="EDGE";;
			 *) MODE="-";;
		esac
	elif [ "$TECH" = 2 ]; then
		TECH1=$(echo "$O" | awk '/^_OWCTI/ {print $2}')
		case $TECH1 in
			1*) MODE="UMTS";;
			2*) MODE="HSDPA";;
			3*) MODE="HSUPA";;
			4*) MODE="HSPA";;
			 *) MODE="-";;
		esac
	fi
fi

# Sierra
if [ "x$MODE" = "x-" ]; then
	TECH=$(echo "$O" | awk -F[,\ ] '/^\*CNTI/ {print $3}' | sed 's|/|,|g')
	if [ "x$TECH" != "x" ]; then
		MODE="$TECH"
	fi
fi

# Novatel
if [ "x$MODE" = "x-" ]; then
	TECH=$(echo "$O" | awk -F[,\ ] '/^\$CNTI/ {print $4}' | sed 's|/|,|g')
	if [ "x$TECH" != "x" ]; then
		MODE="$TECH"
	fi
fi

# Vodafone - icera
if [ "x$MODE" = "x-" ]; then
	TECH=$(echo "$O" | awk -F[,\ ] '/^\%NWSTATE/ {print $4}' | sed 's|/|,|g')
	if [ "x$TECH" != "x" ]; then
		MODE="$TECH"
	fi
fi

# SIMCOM
if [ "x$MODE" = "x-" ]; then
	TECH=$(echo "$O" | awk -F[,\ ] '/^\+CNSMOD/ {print $3}')
	case "$TECH" in
		1*) MODE="GSM";;
		2*) MODE="GPRS";;
		3*) MODE="EDGE";;
		4*) MODE="UMTS";;
		5*) MODE="HSDPA";;
		6*) MODE="HSUPA";;
		7*) MODE="HSPA";;
		 *) MODE="-";;
	esac
fi

# generic 3GPP TS 27.007 V10.4.0
if [ "x$MODE" = "x-" ]; then
	TECH=$(echo "$O" | awk -F[,] '/^\+COPS/ {print $4}')
	case "$TECH" in
		2*) MODE="UMTS";;
		0*|3*) MODE="EDGE";;
		4*) MODE="HSDPA";;
		5*) MODE="HSUPA";;
		6*) MODE="HSPA";;
		7*) MODE="LTE";;
		 *) MODE="-";;
	esac
fi

# CREG
CREG="+CGREG"
LAC=$(echo "$O" | awk -F[,] '/\'$CREG'/ {printf "%s", toupper($3)}' | sed 's/[^A-F0-9]//g')
if [ "x$LAC" = "x" ]; then
	CREG="+CREG"
	LAC=$(echo "$O" | awk -F[,] '/\'$CREG'/ {printf "%s", toupper($3)}' | sed 's/[^A-F0-9]//g')
fi

if [ "x$REGST" = "x" ]; then
	REGST=$(echo "$O" | awk -F[,] '/^\+CREG/ {print $2}')
	case $REGST in
		0) REGST="Not Registered";;
		1) REGST="Registered";;
		2) REGST="Searching";;
		3) REGST="Denied";;
		4) REGST="Unknown";;
		5) REGST="Roaming";;
	esac
fi
			

if [ "x$LAC" != "x" ]; then
	LAC_NUM=$(printf %d 0x$LAC)
else
	LAC="-"
	LAC_NUM="-"
fi

# TAC
TAC=$(echo "$O" | awk -F[,] '/^\+CEREG/ {printf "%s", toupper($3)}' | sed 's/[^A-F0-9]//g')
if [ "x$TAC" != "x" ]; then
	TAC_NUM=$(printf %d 0x$TAC)
else
	TAC="-"
	TAC_NUM="-"
fi


# ECIO / RSCP
ECIO="-"
RSCP="-"
# RSRP / RSRQ
RSRP="-"
RSRQ="-"
SINR="-"
ECIx=$(echo "$O" | awk -F[,\ ] '/^\+ZRSSI:/ {print $3}')
if [ "x$ECIx" != "x" ]; then
	ECIO=`expr $ECIx / 2`
	ECIO="-"$ECIO
fi

RSCx=$(echo "$O" | awk -F[,\ ] '/^\+ZRSSI:/ {print $4}')
	if [ "x$RSCx" != "x" ]; then
		RSCP=`expr $RSCx / 2`
		RSCP="-"$RSCP
fi

RSCx=$(echo "$O" | awk -F[,\ ] '/^\^CSNR:/ {print $2}')
if [ "x$RSCx" != "x" ]; then
	RSCP=$RSCx
	SINR=$RSCP
fi

ECIx=$(echo "$O" | awk -F[,\ ] '/^\^CSNR:/ {print $3}')
if [ "x$ECIx" != "x" ]; then
	ECIO=$ECIx
fi


RSRx=$(echo "$O" | awk -F[,:] '/^\^LTERSRP:/ {print $2}')
if [ "x$RSRx" != "x" ]; then
	RSRP=$RSRx
	RSRQ=$(echo "$O" | awk -F[,:] '/^\^LTERSRP:/ {print $3}')
fi

TECH=$(echo "$O" | awk -F[,:] '/^\^HCSQ:/ {print $2}' | sed 's/[" ]//g')
if [ "x$TECH" != "x" ]; then
	PARAM2=$(echo "$O" | awk -F[,:] '/^\^HCSQ:/ {print $4}')
	PARAM3=$(echo "$O" | awk -F[,:] '/^\^HCSQ:/ {print $5}')
	PARAM4=$(echo "$O" | awk -F[,:] '/^\^HCSQ:/ {print $6}')

	case "$TECH" in
		WCDMA*)
			RSCP=$(awk 'BEGIN {print -121 + '$PARAM2'}')
			ECIO=$(awk 'BEGIN {print -32.5 + '$PARAM3'/2}')
			SINR=$ECIO
			;;
		LTE*)
			RSRP=$(awk 'BEGIN {print -141 + '$PARAM2'}')
			SINR=$(awk 'BEGIN {print -20.2 + '$PARAM3'/5}')
			RSRQ=$(awk 'BEGIN {print -20 + '$PARAM4'/2}')
			;;
	esac
fi

# IMEI number
IMEI=$(echo "$O" | awk -F[:] '/IMEI/ { print $2}')
if [ "x$DEVICE" = "x" ]; then
	DEVICE="-"
fi

if [ -n "$SEC" ]; then
	if [ "x$(uci -q get network.$SEC.proto)" = "xqmi" ]; then
		. /usr/share/libubox/jshn.sh
		json_init
		json_load "$(uqmi -d "$(uci -q get network.$SEC.device)" --get-signal-info)" >/dev/null 2>&1
		json_get_var T type
		if [ "x$T" = "xlte" ]; then
			json_get_var RSRP rsrp
			json_get_var RSRQ rsrq
			json_get_var SINR snr
			json_load "$(uqmi -d "$(uci -q get network.$SEC.device)" --get-serving-system)" >/dev/null 2>&1
		fi
		if [ "x$T" = "xwcdma" ]; then
			json_get_var ECIO ecio
			json_get_var RSSI rssi
			json_get_var RSCP rscp
			SINR=$ECIO
			if [ -z "$RSCP" ]; then
				RSCP=$((RSSI+ECIO))
			fi
			json_load "$(uqmi -d "$(uci -q get network.$SEC.device)" --get-serving-system)" >/dev/null 2>&1
		fi
		if [ "x$T" = "xgsm" ]; then
			json_load "$(uqmi -d "$(uci -q get network.$SEC.device)" --get-serving-system)" >/dev/null 2>&1
		fi

	fi
fi

BTSINFO=""
CID=$(echo "$O" | awk -F[,] '/\'$CREG'/ {printf "%s", toupper($4)}' | sed 's/[^A-F0-9]//g')
if [ "x$CID" != "x" ]; then
	CID_NUM=$(printf %d 0x$CID)

	if [ ${#CID} -gt 4 ]; then
		T=$(echo "$CID" | awk '{print substr($1,length(substr($1,1,length($1)-4))+1)}')
	else
		T=$CID
	fi
else
	CID="-"
	CID_NUM="-"
fi


# Channel Num
if [ "$MODE" = "LTE" ]; then
	# for Quectel modems
	EARFCN=$(echo "$O" |awk -F[,\ ] '/^\+QNWINFO/ {print $8}')
	if [ ! $EARFCN ]; then
		# for SIMCOM modems
		EARFCN=$(echo "$O" | awk -F[,\ ] '/^\+CPSI/ {print $9}')
	fi
	if [ ! $EARFCN ]; then
		# for Huawei.
		EARFCN=$(echo "$O" | awk -F[,\ ] '/^\^HFREQINFO/ {print $5}')
	fi
else
	# for Quectel modems 2G/3G networks
	EARFCN=$(echo "$O" |awk -F[,\ ] '/^\+QNWINFO/ {print $6}')
	if [ ! $EARFCN ]; then
		# for SimCom modems 2G/3G networks
		EARFCN=$(echo "$O" | awk -F[,\ ] '/^\+CPSI/ {print $11}')
	fi
	if [ ! $EARFCN ]; then
		# for Huawei. Not fully support any models for all bands! Maybe repair it?
		EARFCN=$(echo "$O" | awk -F[,\ ] '/^\^HFREQINFO/ {print $5}')
		if [ ! $EARFCN ]; then
			EARFCN="not support"
		fi
	fi
fi

# ARFCN and SNR name 
case $MODE in
	LTE)
		CNNAME="EARFCN"
		SNRNAME="SINR"
	;;
	UMTS|HSPA|HSUPA|HSDPA)
		CNNAME="UARFCN"
		SNRNAME="ECIO"
	;;
	*)
		CNNAME="ARFCN"
		SNRNAME="SINR/ECIO"
	;;
esac

# Device name
DEVICE=$(echo "$O" | awk -F[:] '/DEVICE/ { print $2}')
if [ "x$DEVICE" = "x" ]; then
	DEVICE="-"
fi


freq_band
json_status

exit 0

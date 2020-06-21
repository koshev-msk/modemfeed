#!/bin/sh
if -r /usr/share/modeminfo/scripts/modeminfo; then
	source . /usr/share/modeminfo/scripts/modeminfo
else
	exit 0
fi

case $1 in
	firstinstall)
		get_device_info
	;;
	*)
		get_device_info
		get_data_in
		if_null
		json_status
	;;
esac

exit 0

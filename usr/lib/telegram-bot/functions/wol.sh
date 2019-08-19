if [ -z "$1" ]; then
	echo "MAC address is empty"
else
	/usr/bin/etherwake -D -i 'br-lan' "$1"
fi

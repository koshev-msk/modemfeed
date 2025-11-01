#!/bin/sh

usage() {
	echo "Usage: $0 -i <interface1> [interface2 ...] -l <led1> [led2 ...] [-o <led_off1> [led_off2 ...]]"
	exit 1
}

# Parse arguments
parse_args() {
	while [ $# -gt 0 ]; do
		case "$1" in
			-i)
				shift
				while [ $# -gt 0 ] && [ "$(echo "$1" | cut -c1)" != "-" ]; do
					interfaces="$interfaces $1"
					shift
				done
			;;
			-l)
				shift
				while [ $# -gt 0 ] && [ "$(echo "$1" | cut -c1)" != "-" ]; do
					leds_on="$leds_on $1"
					shift
				done
			;;
			-o)
				shift
				while [ $# -gt 0 ] && [ "$(echo "$1" | cut -c1)" != "-" ]; do
					leds_off="$leds_off $1"
					shift
				done
			;;
			*)
				usage
			;;
		esac
	done
}

# Validate and adjust parameters
check_counts() {
	# Count interfaces and LEDs
	count_int=$(echo "$interfaces" | awk '{print NF}')
	count_on=$(echo "$leds_on" | awk '{print NF}')

	if [ "$count_int" -eq 0 ]; then
		echo "Error: at least one interface must be specified"
		usage
	fi

	if [ "$count_on" -eq 0 ]; then
		echo "Error: at least one LED must be specified with -l"
		usage
	fi

	# If more LEDs than interfaces, limit LEDs to match interfaces
	if [ "$count_on" -gt "$count_int" ]; then
		leds_on=$(echo "$leds_on" | awk -v n="$count_int" '{for(i=1;i<=n;i++) printf "%s ", $i; print ""}')
		count_on="$count_int"
	fi

	# If more off LEDs than interfaces, limit off LEDs to match interfaces
	if [ -n "$leds_off" ]; then
		count_off=$(echo "$leds_off" | awk '{print NF}')
		if [ "$count_off" -gt "$count_int" ]; then
			leds_off=$(echo "$leds_off" | awk -v n="$count_int" '{for(i=1;i<=n;i++) printf "%s ", $i; print ""}')
		fi
	fi
}

# Generate configuration
generate_config() {
	echo "$interfaces" | awk -v leds_on="$leds_on" -v leds_off="$leds_off" '
	BEGIN {
		split(leds_on, on_arr, " ")
		split(leds_off, off_arr, " ")
	}
	{
		for (i = 1; i <= NF; i++) {
			if (leds_on != "" && i <= length(on_arr) || leds_off != "" && i <= length(off_arr)) {
				print "config led '\''" $i "'\''"
				print "	option iface '\''" $i "'\''"
				if (leds_on != "" && i <= length(on_arr)) {
					print "	option led_on '\''" on_arr[i] "'\''"
				}
				if (leds_off != "" && i <= length(off_arr)) {
					print "	option led_off '\''" off_arr[i] "'\''"
				}
			}
			print ""
		}
	}'
}

# Main program
main() {
	if [ $# -eq 0 ]; then
		usage
	fi

	parse_args "$@"
	check_counts
	generate_config
}

main "$@"


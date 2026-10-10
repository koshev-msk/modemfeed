#!/bin/sh
# ab-stats.sh: ststistic collect for mwan3-autobalancer. One CSV-line per probe/.
#   cron:  * * * * * /root/ab-stats.sh >/dev/null 2>&1
#   run: sh ab-stats.sh        (one check)      sh ab-stats.sh loop   (check in INTERVAL s)
# Result in /tmp (RAM), up to MAXLINES lines.

OUT=${OUT:-/tmp/ab-stats.csv}
INTERVAL=${INTERVAL:-60}
MAXLINES=${MAXLINES:-8000}           #  approximately 5 days шт 60s step
SLACK=${SLACK:-200}                  # truncate, if lines above MAXLINES + SLACK
CHAIN=${CHAIN:-mwan3_policy_balanced}
PAIRS=${PAIRS:-"modem1:wwan0 modem2:wwan1"}   #  mwan3 logical_iface:physical_iface

chain_state() {   # "mod/width:width", e..g 20/8:12; "-" if vmap not exist
	nft list chain inet mwan3 "$CHAIN" 2>/dev/null | awk '
		/vmap/ {
			m = ""; for (i = 1; i <= NF; i++) if ($i == "mod") m = $(i + 1)
			s = $0; sub(/.*\{/, "", s); n = split(s, p, ","); w = ""
			for (i = 1; i <= n; i++) {
				split(p[i], r, " "); t = r[1]
				if (t ~ /^[0-9]+-[0-9]+$/) { split(t, q, "-"); x = q[2] - q[1] + 1 } else x = 1
				w = w (w == "" ? "" : ":") x
			}
			print m "/" w; found = 1
		}
		END { if (!found) print "-" }'
}

ct_marks() {      # connections per id iface (bits 8..13 tags): "2=130;3=118"
	awk '{ for (i = 1; i <= NF; i++) if ($i ~ /^mark=/) {
		id = int((substr($i, 6) + 0) / 256) % 64; if (id > 0) c[id]++ } }
	     END { o = ""; for (k = 1; k <= 63; k++) if (c[k]) o = o (o == "" ? "" : ";") k "=" c[k]; print (o == "" ? "-" : o) }' \
		/proc/net/nf_conntrack 2>/dev/null
}

daemon_cost() {   # "RSS_КБ,tics_CPU" mwan3-autobalancer daemon
	pid=
	for p in /proc/[0-9]*; do
		case "$(tr '\0' ' ' < "$p/cmdline" 2>/dev/null)" in
			*mwan3-autobalancer*run*) pid=${p#/proc/}; break ;;
		esac
	done
	[ -n "$pid" ] || { echo ","; return; }
	rss=$(awk '/VmRSS/ { print $2 }' /proc/$pid/status 2>/dev/null)
	cpu=$(awk '{ print $14 + $15 }' /proc/$pid/stat 2>/dev/null)
	echo "$rss,$cpu"
}

header() {
	h="epoch,time,chain,conns,rss_kb,cpu_ticks"
	for pr in $PAIRS; do i=${pr%%:*}; h="$h,$i.status,$i.latency,$i.loss,$i.rx,$i.tx"; done
	echo "$h"
}

row() {
	J=$(ubus call mwan3 status '{"section":"interfaces"}' 2>/dev/null)
	r="$(date +%s),$(date '+%Y-%m-%dT%H:%M:%S'),$(chain_state),$(ct_marks),$(daemon_cost)"
	for pr in $PAIRS; do
		i=${pr%%:*}; d=${pr##*:}
		st=$(jsonfilter -s "$J" -e "@.interfaces.$i.status" 2>/dev/null)
		la=$(jsonfilter -s "$J" -e "@.interfaces.$i.track_ip[0].latency" 2>/dev/null)
		lo=$(jsonfilter -s "$J" -e "@.interfaces.$i.track_ip[0].packetloss" 2>/dev/null)
		rx=$(cat /sys/class/net/$d/statistics/rx_bytes 2>/dev/null)
		tx=$(cat /sys/class/net/$d/statistics/tx_bytes 2>/dev/null)
		r="$r,$st,$la,$lo,$rx,$tx"
	done
	echo "$r" >> "$OUT"
}

trim() {
	[ "$(wc -l < "$OUT")" -gt $((MAXLINES + SLACK)) ] || return 0
	tail -n "$MAXLINES" "$OUT" > "$OUT.tmp" && cat "$OUT.tmp" > "$OUT"
	rm -f "$OUT.tmp"
}

sample() {
	[ -s "$OUT" ] || header > "$OUT"
	row
	trim
}

if [ "$1" = loop ]; then
	while :; do sample; sleep "$INTERVAL"; done
else
	sample
fi

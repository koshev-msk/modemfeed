#!/bin/sh
# OTA upgrade background launcher
# Called by LuCI to start upgrade asynchronously.
# Returns immediately so the RPC call completes and JS polling can work.

rm -f /tmp/ota_progress
setsid /usr/share/ota.sh upgrade > /tmp/ota_upgrade.log 2>&1 &
echo $! > /tmp/ota_upgrade.pid

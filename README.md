# luci-app-rtorrent
rTorrent client for OpenWrt's LuCI web interface

**Install instructions**

1.) Install rtorrent-rpc (available in snapshots repository)
```
opkg update
IPKG_NO_SCRIPT=1 opkg install http://downloads.openwrt.org/snapshots/trunk/ar71xx/generic/packages/packages/libtorrent_0.13.4-git-0-72e908707f01ee01a9b4918436c64348878b63f7_ar71xx.ipk
IPKG_NO_SCRIPT=1 opkg install http://downloads.openwrt.org/snapshots/trunk/ar71xx/generic/packages/packages/rtorrent-rpc_0.9.4-git-0-7343e33a6a0d279179b304a380bf011f1c8be64a_ar71xx.ipk
```
following dependent packages were also installed: _libpthread, libpolarssl, libcurl, xmlrpc-c-common, xmlrpc-c-internal, xmlrpc-c, libstdcpp, xmlrpc-c-server, libsigcxx, zlib libopenssl, libncursesw_

2.) Install libncurses
```
opkg install libncurses
```

3.) Create rTorrent config file

Minimal _/root/.rtorrent.rc_ file:
```
directory = /path/to/downloads/
session = /path/to/session/
scgi_port = 127.0.0.1:5000
```
Sample _/root/.rtorrent.rc_ file:
http://pissedoffadmins.com/os/linux/sample-rtorrent-rc-file.html

4.) Create init.d script (optional)

Install screen
```
opkg install screen
```

Create _/etc/init.d/rtorrent_ script
```
#!/bin/sh /etc/rc.common

START=99

start() {
  sleep 3
  screen -dm -t rtorrent nice -19 rtorrent
}

stop() {
  killall rtorrent
}
```

Start rtorrent
```
chmod +x /etc/init.d/rtorrent
/etc/init.d/rtorrent enable
/etc/init.d/rtorrent start
```

5.) Install wget

(the wget in  busybox does not support https)
```
opkg install wget ca-certificates
```

6.) Install luci-app-rtorrent
```
echo '' > /etc/opkg.conf
opkg update
opkg install luci-app-rtorrent
```

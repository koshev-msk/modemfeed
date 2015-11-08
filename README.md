# luci-app-rtorrent
rTorrent client for OpenWrt's LuCI web interface

## Screenshots
[luci-app-rtorrent 0.1.1](https://github.com/wolandmaster/luci-app-rtorrent/wiki/Screenshots)

## Install instructions
(for Openwrt 15.05 Chaos Calmer)

### Install rtorrent-rpc
```
opkg update
opkg install rtorrent-rpc
```

### Create rTorrent config file

#### Minimal _/root/.rtorrent.rc_ file:
```
directory = /path/to/downloads/
session = /path/to/session/
scgi_port = 127.0.0.1:5000

schedule = rss_downloader,300,300,"execute=/usr/lib/lua/rss_downloader.lua"
```
#### Sample _/root/.rtorrent.rc_ file:
http://pissedoffadmins.com/os/linux/sample-rtorrent-rc-file.html

### Create init.d script (optional)

#### Install screen
```
opkg install screen
```

#### Create _/etc/init.d/rtorrent_ script
```
#!/bin/sh /etc/rc.common

START=99
STOP=99

SCREEN=/usr/sbin/screen
PROG=/usr/bin/rtorrent

start() {
  sleep 3
  $SCREEN -dm -t rtorrent nice -19 $PROG
}

stop() {
  killall rtorrent
}
```

#### Start rtorrent
```
chmod +x /etc/init.d/rtorrent
/etc/init.d/rtorrent enable
/etc/init.d/rtorrent start
```

### Install wget
(the wget in busybox does not support https)
```
opkg install ca-certificates
opkg install wget
ln -sf $(which wget-ssl) /usr/bin/wget
```

### Install luci-app-rtorrent
```
wget -nv https://github.com/wolandmaster/luci-app-rtorrent/releases/download/latest/e1a1ba8004c4220f -O /etc/opkg/keys/e1a1ba8004c4220f
echo 'src/gz luci_app_rtorrent https://github.com/wolandmaster/luci-app-rtorrent/releases/download/latest' >> /etc/opkg.conf
opkg update
opkg install luci-app-rtorrent
```

### Upgrade installed version
```
opkg update
opkg upgrade luci-app-rtorrent
```

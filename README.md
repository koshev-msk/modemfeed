# cellLED
OpenWrt  application for showing cellular RSSI LED's

Support import information an modemmanager (mmcli), qmi-utils, uqmi and serial port.

Support LED Linear and RGB PWM modes.

# How-To compile

Add repo in feeds.conf.default OpenWrt SDK 
```
src-git cellled https://github.com/koshev-msk/cellled.git
```
Update feed and compile
```
./scriptps/feeds update -a && ./scripts/feeds install -a
make -j$((`nproc`+1))  package/feeds/cellled/luci-app-cellled/compile
```

# modemfeed

Is a repository for OpenWrt firmware worked by with LTE cellular modems.

Included next packages:

|Package       |        Dependies      |       Description        |
|:-------------|:----------------------|:-------------------------|
| luci-app-modeminfo|modeminfo|Dashboard for LTE modemds.|
|luci-app-smstools3|smstools3|web UI smstools3 package.|
|luci-app-mmcomig|modemmanager|band manipulation modem via mmcli utility.|
|luci-app-atinout|atinout|AT commands tool.|
|luci-app-cellled|cellled|LED cellular signal signal strength.|
|luci-app-ttl|iptables-mod-ipopt,kmod-ipt-ipopt,kmod-ipt-nat6|TTL Change utility.|
|qtools|libc|tools manipulation Qualcomm chipset cellualr modems.|
|asterisk-chan-quectel|asterisk|asterisk plugin for SimCom and Quectel modems.|
|xmm-modem|kmod-usb-net-ncm, kmod-usb-acm|Intel XMM modem connect scripts|
* and more packages not included in official OpenWrt Repo.

# How-to add repo and compile packages

Add next line to feeds.conf.default in OpenWrt SDK/Buildroot

```
src-git modemfeed https://github.com/koshev-msk/modemfeed.git
```

Update feeds and compile singe package

```
./scripts/feeds update -a; ./scripts/feeds install -a
make -j$((`nproc` + 1)) package/feeds/modemfeed/<package_name>/compile
```

or `make menuconfig` menu to include package(s) firmware in Buildroot

# Precompiled packages

http://openwrt.132lan.ru/packages/21.02/packages/

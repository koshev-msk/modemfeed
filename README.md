# modemfeed

Ia a repository for OpenWrt wirmware worked by with LTE celluar modems.

Included next packages:

* modeminfo (luci-app-modeminfo) - Dashboard for LTE modemds.

* luci-app-smstools3 - web UI smstools3 package.

* luci-app-mmcomig - band manipulation modem via mmcli utility.

* atinout (luci-app-atinout) - AT commands tool.

* qtools - tools manupulation Qualcomm chipset cellualr modems.

* and more packages not included in official OpenWrt Repo.

# How-to add repo and compile packages

Add next line to feeds.conf.default in OpenWrt SDK/Buildroot

```
src-gz modemfeed https://github.com/koshev-msk/modemfeed.git
./scripts update -a; ./scrips/install -a
make -j$((`nproc+1`)) package/feeds/modemfeed/*package*/compile
```

or `make menuconfig` to include package firmware in Buildroot

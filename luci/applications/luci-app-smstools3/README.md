<<<<<<< HEAD
# luci-app-atinout-mod

Web UI AT-commands using atinout for OpenWrt.
My luci-app-atinout-mod is modyfication of https://github.com/koshev-msk/luci-app-atinout.

![GitHub release (latest by date)](https://img.shields.io/github/v/release/4IceG/luci-app-atinout-mod?style=flat-square)
![GitHub stars](https://img.shields.io/github/stars/4IceG/luci-app-atinout-mod?style=flat-square)
![GitHub forks](https://img.shields.io/github/forks/4IceG/luci-app-atinout-mod?style=flat-square)
![GitHub All Releases](https://img.shields.io/github/downloads/4IceG/luci-app-atinout-mod/total)

### Preview and quick configuration (modem Quectel EM12-G) / Podgląd oraz szybka konfiguracja (modem Quectel EM12-G)

![](https://raw.githubusercontent.com/4IceG/Personal_data/master/atcommands.gif)
=======
# luci-app-smstools3

Web UI smstools3 for OpenWrt LuCI.
How-to compile:
```
cd feeds/luci/applications/
git clone https://github.com/koshev-msk/luci-app-smstools3.git
cd ../../..
./scripts/feeds update -a; ./scripts/feeds install -a
make -j $(($(nproc)+1)) package/feeds/luci/luci-app-smstools3/compile
```

Note: If you use this app with modemmanager, please move or remove /etc/hotplug.d/tty/25-modemmanager-tty

<details>
   <summary>Screenshots</summary>
   
   ![](https://raw.githubusercontent.com/koshev-msk/luci-app-smstools3/master/screenshots/incoming.png)
   
   ![](https://raw.githubusercontent.com/koshev-msk/luci-app-smstools3/master/screenshots/outcoming.png)
   
   ![](https://raw.githubusercontent.com/koshev-msk/luci-app-smstools3/master/screenshots/push.png)
   
   ![](https://raw.githubusercontent.com/koshev-msk/luci-app-smstools3/master/screenshots/setup.png)
   
</details>
>>>>>>> luci-app-smstools3/master

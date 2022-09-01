# OpenWrt scripts to configure connection Fibocom L860-GL
Intel XMM 7650 LTE-A Pro modem.

# How-to compile package
```
cd feeds/package/net/
git clone https://github.com/koshev-msk/xmm-modem.git
cd ../../..
./scripts feeds update -a; ./scripts/feeds install -a
make -j $(($(nproc)+1)) package/feeds/packages/xmm-modem/compile
```

# How-to configure cellular connection
The config stored in /etc/config/xmm-modem. Example configuration:
```
config xmm-modem
	option enable '1' # Enable connect scenario
	option device '/dev/ttyACM0' # Device serial port
	option apn 'internet' # ISP Access 
```

# How-to configure interface
Create new unmanaged interface, select physical device eth1 or usb0 or wwan0.
Setup force link option `option force_link '1'`

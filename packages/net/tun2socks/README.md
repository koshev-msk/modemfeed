# OpenWrt netifd protocol handler for tun2socks go application

# How-to configure tun2socks interface
The config stored in /etc/config/network. Example configuration:

```
config interface 't2s'
	option proto 't2s'
	option ipaddr '10.0.0.10'
	option netmask '255.255.0.0'
	option gateway '10.0.0.1' # optional
	option proxy 'socks5' # type proxy
	option host 'my.socks5.proxy:port' # upstream proxy-server
	option username 'username' # username 
	option password 'password' # password

```

# How-to configure interface via LuCi
build and install package luci-proto-tun2socks


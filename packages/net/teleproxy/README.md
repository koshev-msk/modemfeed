
# MTPROTO proxy server

teleproxy OpenWrt package

example config file

```
config teleproxy 'default'
  option address '0.0.0.0'      # listen ipaddr or '::' with ipv6
  option ipv6 '1'               # Use IPv6 proto
  option port '8443'            # proxy port
  option direct '1'             # connect directly to Telegram DCs instead of through ME relays
  option aes_pwd 'file'         # sets custom secret.conf file
  option socks 'addr:port'      # route upstream DC connections through SOCKS5 proxy
  option socks_auth 'user:pass' # autentification on SOCKS5 upstream proxy
  option extra '--extra args'   # exta args to run teleproxy. See "teleproxy --help".
```

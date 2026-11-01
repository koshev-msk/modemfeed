# vless-core-cli

OpenWrt package for [notfence/vless-core-cli](https://github.com/notfence/vless-core-cli) —
a standalone C client that connects to a VLESS, Trojan, VMess, Shadowsocks or SOCKS5 server
and exposes a local SOCKS5 proxy.

Compared to Go-based cores it is very small and depends only on `libopenssl`,
which makes it suitable for devices with small flash and RAM.

Supported upstreams:

- VLESS (with `xtls-rprx-vision` and VLESS Encryption), Trojan, VMess AEAD (`alterId=0`),
  Shadowsocks (AEAD and 2022) over TCP / WebSocket / gRPC / XHTTP / HTTPUpgrade with
  no security, TLS or Reality (Reality is not available for WebSocket and HTTPUpgrade).
- Plain SOCKS5, optionally with username/password.

## Files

| Path | Purpose |
|------|---------|
| `/usr/bin/vless-core` | client binary |
| `/etc/init.d/vless-core` | procd init script |
| `/etc/config/vless-core` | UCI config |

## Quick start

```sh
uci set vless-core.main.uri='vless://UUID@example.com:443?type=tcp&security=reality&sni=www.example.org&fp=chrome&pbk=PUBLIC_KEY&sid=SHORT_ID&flow=xtls-rprx-vision#server'
uci set vless-core.main.listen_port='1080'
uci set vless-core.main.enabled='1'
uci commit vless-core
/etc/init.d/vless-core enable
/etc/init.d/vless-core start
```

Check:

```sh
curl -x socks5h://127.0.0.1:1080 https://2ip.io
```

The SOCKS5 listener binds to `127.0.0.1` only. Use it with tun2socks, redsocks or anything
else that can talk to a SOCKS5 proxy. Logs go to `logread`.

## UCI options

Section `config vless-core 'main'`:

| Option | Default | Description |
|--------|---------|-------------|
| `enabled` | `0` | Start the service (`1`/`0`) |
| `uri` | – | `vless://`, `vmess://`, `trojan://`, `ss://` or `socks5://` upstream URI (required) |
| `listen_port` | `1080` | Local SOCKS5 port on `127.0.0.1` |
| `routing` | – | Optional routing rules, see below |
| `tls_mode` | `strict` | Certificate verification: `strict`, `insecure` or `tofu` |

Always quote the URI in `uci set`: it contains `&`, `#` and `?`.

The client serves exactly one upstream URI, so the init script runs a single instance.

## URI examples

Reality + Vision:

```
vless://UUID@example.com:443?type=tcp&security=reality&sni=www.example.org&fp=chrome&pbk=PUBLIC_KEY&sid=SHORT_ID&flow=xtls-rprx-vision#reality
```

XHTTP + TLS:

```
vless://UUID@example.com:443?type=xhttp&security=tls&sni=example.com&path=%2Fxh&mode=auto#xhttp
```

WebSocket + TLS:

```
vless://UUID@example.com:443?type=ws&security=tls&sni=example.com&host=example.com&path=%2Fws#ws
```

gRPC + Reality:

```
vless://UUID@example.com:443?type=grpc&security=reality&serviceName=svc&sni=www.example.org&fp=chrome&pbk=PUBLIC_KEY&sid=SHORT_ID#grpc
```

HTTPUpgrade + TLS:

```
vless://UUID@example.com:443?type=httpupgrade&security=tls&sni=example.com&host=example.com&path=%2Fup#httpupgrade
```

Trojan (TLS by default, the whole userinfo is the password):

```
trojan://PASSWORD@example.com:443?sni=example.com#trojan
```

VMess (`encryption` is the body mode, `scy` is an alias; the base64 JSON form is accepted too):

```
vmess://UUID@example.com:443?type=ws&security=tls&sni=example.com&host=example.com&path=%2Fws&encryption=auto#vmess
```

Shadowsocks (SIP002, userinfo is base64url of `method:password`):

```
ss://BASE64URL_METHOD_PASSWORD@example.com:8388#ss
```

Upstream SOCKS5:

```
socks5://192.0.2.1:1080
```

Supported `fp` values: `chrome`, `firefox`, `edge`, `random`, `randomized`, `qq`
(there are no `safari` or `ios` fingerprints).

## Routing

`routing` is a single string, fields separated by `;`:

```
ENABLED;DEFAULT;LAN;action,type,value;action,type,value;...
```

| Field | Values |
|-------|--------|
| `ENABLED` | `1` / `0` |
| `DEFAULT` | `proxy` / `direct` / `block` — used when no rule matches |
| `LAN` | `1` / `0` — private IP ranges always go `direct` (checked before the rules) |
| `action` | `proxy` / `direct` / `block` |
| `type` | `domain` (exact), `suffix` (domain and subdomains), `cidr`, `port` (single or `a-b`) |

Rules are checked in order, first match wins. Up to 48 rules, 8191 characters total.
Domains are lowercase `a-z0-9-_.` without leading or trailing dot (`ru`, not `.ru`).

Example: everything through the proxy, `.ru` and `.lan` direct, SMTP and one subnet blocked:

```sh
uci set vless-core.main.routing='1;proxy;1;direct,suffix,ru;direct,suffix,lan;block,port,25;block,cidr,203.0.113.0/24'
uci commit vless-core
/etc/init.d/vless-core restart
```

Notes:

- Domain rules match when the client sends a hostname (`socks5h`). For a bare IP the client
  tries to sniff the domain from the first packet, but only if domain/suffix rules exist.
- `8.8.8.8:53` always goes through the proxy if no rule matched earlier, even with `DEFAULT=direct`.
- A `direct` connection is opened by `vless-core` itself via the normal routing table. If your
  default route points into a tun device (tun2socks), direct traffic will loop back into it.
  The client has `--route-control-port` / `--route-control-socket` for an external controller,
  they are not exposed in UCI.

## TLS verification

`tls_mode` is passed to the client as `VLESS_TLS_MODE`:

- `strict` — only verified TLS, no fallback (default). For `security=tls` install `ca-bundle`.
- `insecure` — never verify certificates.
- `tofu` — pin the certificate on first connection, later connections must match.

## Reality client version

For Reality the client writes an Xray version (`x.y.z`, default `26.3.27`) into the ClientHello
session ID. Override it with the `XRAY_VER` environment variable or `--xray-version x.y.z`,
for example if the server limits client versions with `minClientVer` / `maxClientVer`.
This is not exposed in UCI.

## Build notes

Upstream's Makefile runs its test suite (clang sanitizers, a separate OpenSSL 3.5, Xray) and
an x86-64-only binary check, and hardcodes the output name `vless-core-linux-amd64`.
The package therefore does not use it: it compiles `src/*.c` directly with the SDK toolchain
and installs the result as `/usr/bin/vless-core`.

ML-KEM based features (VLESS Encryption, the X25519MLKEM768 key share) need OpenSSL 3.5 or newer.

Tested: OpenWrt SDK, musl x86-64, OpenSSL 3.5.9.

## ToDo

- `luci-app-vless-core` — LuCI web interface (someday).

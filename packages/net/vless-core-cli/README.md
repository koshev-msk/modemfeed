# vless-core-cli

OpenWrt package for [notfence/vless-core-cli](https://github.com/notfence/vless-core-cli) —
a standalone C client for VLESS that exposes a local SOCKS5 proxy.

Compared to Go-based cores it is tiny (the ipk is about ~70 KB) and depends only on `libopenssl`,
which makes it suitable for devices with small flash and RAM.

Supported upstreams: VLESS over TCP / XHTTP / gRPC / WebSocket with Reality, TLS or no security
(`xtls-rprx-vision` on TCP), and plain SOCKS5.

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
| `uri` | – | `vless://...` or `socks5://host:port` upstream URI (required) |
| `listen_port` | `1080` | Local SOCKS5 port on `127.0.0.1` |
| `routing` | – | Optional routing rules, see below |
| `tls_mode` | `strict` | Certificate verification: `strict`, `insecure` or `tofu` |

Always quote the URI in `uci set`: it contains `&`, `#` and `?`.

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

Upstream SOCKS5:

```
socks5://192.0.2.1:1080
```

Supported `fp` values: `chrome`, `firefox`, `edge`, `random`, `randomized`, `qq`.

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

## Build notes

Upstream hardcodes the output name `vless-core-linux-amd64` regardless of the target
architecture, so the package passes `BIN_LINUX=vless-core` to upstream `make` and builds
with the SDK `CC`/`CFLAGS`/`LDFLAGS`.

## ToDo

- `luci-app-vless-core` — LuCI web interface.

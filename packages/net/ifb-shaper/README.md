# ifb-shaper

Per-client traffic shaper for OpenWrt based on HTB and IFB, configured via UCI.

- **Download**: HTB on the egress of the interface's device (e.g. `br-lan`), clients matched by destination address.
- **Upload**: device ingress is redirected to `ifb-<device>`, HTB on the IFB, clients matched by source address.
- IPv4 and IPv6, several addresses per client.
- Each client gets its own class with five priority sub-classes (both directions), from highest to lowest priority: HTTP(S) (tcp/80, tcp/443), other TCP, other UDP, QUIC (udp/443), everything else (ICMP etc.).
- `fq_codel` on every leaf class by default (configurable).

## How it works

A `shaper` section is bound to a **logical netifd interface**. Its L3 device is resolved at run time and the IFB is created automatically as `ifb-<device>` (max. 15 chars, so the device name may be up to 11 chars). The shaper is re-applied when the interface changes state or the config is changed.

Class layout (per device and per IFB):

| Class | Purpose |
|-------|---------|
| `1:1` | root, `total` |
| `1:10` | bypass (local) networks, `local_rate` / `local_ceil` |
| `1:1000` | parent of all client classes, `down_total` / `up_total` |
| `1:9000` | default (unclassified), `default_rate` / `default_ceil` |
| `1:<n>1` | a client, numbered automatically (max. 2048 per shaper) |
| `1:<n>2`..`1:<n>6` | client sub-classes: HTTP(S), TCP, UDP, QUIC, rest |

Networks listed in `bypass` skip the client classes and go to `1:10`. Note that `1:10` is still capped by `local_ceil` and shared by all clients; set it equal to `total` for effectively unrestricted local traffic.

## Configuration

`/etc/config/ifb-shaper`:

```
config shaper 'lan'
	option interface 'lan'
	list bypass '192.168.1.0/24'

config client 'laptop'
	option shaper 'lan'
	list address '192.168.1.100'
	list address '2001:db8:1::100'
	option down_rate '1Mbit'
	option down_ceil '50Mbit'
	option up_rate '1Mbit'
	option up_ceil '20Mbit'
```

### `shaper`

| Option | Default | Description |
|--------|---------|-------------|
| `enabled` | `1` | Enable/disable this shaper |
| `interface` | – | Logical interface to shape (required) |
| `bypass` | – | List of networks (IPv4/IPv6) not subject to client limits |
| `leaf_qdisc` | `fq_codel limit 1024 flows 128` | qdisc attached to every client sub-class (`tc` arguments); `none` disables it |
| `total` | `1000Mbit` | Root class rate |
| `local_rate` / `local_ceil` | `50Mbit` / `500Mbit` | Bypass class |
| `down_total` / `up_total` | `400Mbit` / `300Mbit` | Aggregate rate of all clients |
| `default_rate` / `default_ceil` | `1Mbit` / `15Mbit` | Unclassified traffic |

### `client`

| Option | Default | Description |
|--------|---------|-------------|
| `shaper` | – | Logical interface name of the shaper (required) |
| `address` | – | Host or network, IPv4 or IPv6; may be a list (required) |
| `down_rate` / `down_ceil` | `1Mbit` / – | Guaranteed / maximum download (`down_ceil` required) |
| `up_rate` / `up_ceil` | `1Mbit` / – | Guaranteed / maximum upload (`up_ceil` required) |

Rates use `tc` notation (`kbit`, `Mbit`, ...). Clients are matched in file order, so put specific hosts **before** wide subnets that contain them.

## Notes

- The ingress hook of the device may be shared with other tools (`clsact` from qosify etc.). It is never deleted or replaced: if there is no `ingress`/`clsact` qdisc, `clsact` is created, and only the shaper's own redirect filters (prio 4900 for IPv4, 4901 for IPv6) are managed. After `stop` an empty `clsact` stays on the device.
- Software/hardware flow offloading can bypass tc for offloaded flows.

## Usage

```
/etc/init.d/ifb-shaper enable
/etc/init.d/ifb-shaper start|stop|reload
/etc/init.d/ifb-shaper dump      # print the tc commands without applying
logread -e ifb-shaper            # errors reported by tc
```

## Dependencies

`tc`, `ip`, `kmod-ifb` (pulls in `kmod-sched-core`).

## Todo

- Luci app.

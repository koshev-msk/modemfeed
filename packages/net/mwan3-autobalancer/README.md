# mwan3-autobalancer

Adaptive weight controller for [`mwan3-nft`](https://github.com/koshev-msk/mwan3-nft) (OpenWrt 25.12, nftables).
It adjusts load-balancing weights at runtime from link quality. It does **not** patch mwan3 and does **not** run any active probes.
Is ucode and shell implementation of https://github.com/AlexStarc/mwan3-autobalancer

## How it works

1. Reads the latency and packet loss that `mwan3track` already collects (via `ubus call mwan3 status`).
2. Smooths the samples (EWMA) and computes a quality factor per link, **relative to that link's own baseline latency**. A single lost ping barely changes anything.
3. Scales each member's configured uci `weight` by its quality factor and rounds the result to a small total (default 20, i.e. 5% steps).
4. If the result differs enough from the live chain (hysteresis), rewrites the `numgen ... vmap` rule of `mwan3_policy_<name>` in **one atomic nft transaction** (`flush` + `add` in a single batch). All other rules, including last-resort, are re-added unchanged. Both `numgen inc` and `numgen random` are supported.

## Safety

- **Observe mode is the default**: decisions are logged, nothing is changed.
- **Level-triggered**: if mwan3 rebuilds the chain (ifup/ifdown), the next pass or the hotplug poke restores the adjusted weights.
- Unrecognized chain shape, unknown member or missing data: the chain is left untouched.
- On stop (SIGTERM) the original uci weights are restored. procd respawns the daemon after a crash.

## Requirements

- `mwan3-nft`, `ucode` with the `fs`, `uci`, `ubus` and `uloop` modules (ucode from OpenWrt 25.12 or newer).
- `check_quality` enabled on the balanced interfaces. To keep mwan3's own up/down decisions unchanged, neutralize its thresholds:

```sh
for i in wan1 wan2; do          # your interface names
	uci set mwan3.$i.check_quality='1'
	uci set mwan3.$i.failure_latency='100000'
	uci set mwan3.$i.recovery_latency='99999'
	uci set mwan3.$i.failure_loss='100'
	uci set mwan3.$i.recovery_loss='99'
done
uci commit mwan3 && mwan3 restart
```

## Usage

```sh
/usr/sbin/mwan3-autobalancer once       # one-shot calculation printout, changes nothing
/usr/sbin/mwan3-autobalancer restore    # restore original uci weights
/etc/init.d/mwan3-autobalancer start    # run the daemon (needs enabled=1)
```

Recommended rollout: `enabled=1`, keep `observe=1`, watch `logread | grep -i autobalancer`, then set `observe=0`.

## Configuration (`/etc/config/mwan3_autobalancer`)

| Option     | Default    | Meaning                                                            |
|------------|------------|--------------------------------------------------------------------|
| `enabled`  | `0`        | Start the daemon                                                   |
| `observe`  | `1`        | Log decisions only, do not touch the chain                         |
| `policy`   | `balanced` | mwan3 policy name                                                  |
| `interval` | `30`       | Seconds between passes (minimum 5)                                 |
| `total`    | `20`       | Sum of integer weights in the chain (step = 100/total %)           |
| `hyst`     | `1.0`      | Apply when a target is off by at least this many steps             |

With `numgen random` finer steps are fine, e.g. `total=100`, `hyst=5`. With `numgen inc` keep `total` small, because connections are handed out in runs of up to `total`.

Model constants (baseline drift, loss threshold, smoothing) are at the top of `model.uc`.

## ubus

Object `mwan3_autobalancer`:

- `poke`: trigger an early pass (called from the hotplug script after mwan3 rebuilds the chain)
- `status`: current mode, policy and last decision

## ToDo

- LuCI page.

## Statistics collector (optional)

`ab-stats.sh` appends one CSV row per run: chain weights, per-link status/latency/loss, interface byte counters, connection counts per mark, daemon RSS and CPU ticks.

```sh
cp ab-stats.sh /root/ && chmod +x /root/ab-stats.sh
(crontab -l 2>/dev/null; echo '* * * * * /root/ab-stats.sh >/dev/null 2>&1') | crontab -
/etc/init.d/cron enable && /etc/init.d/cron restart
```

Data goes to `/tmp/ab-stats.csv` (RAM, trimmed to the last 8000 rows, lost on reboot).

## Thanks
To idea thakns https://github.com/AlexStarc

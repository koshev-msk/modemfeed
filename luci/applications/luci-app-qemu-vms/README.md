# luci-app-qemu-vms

Web UI for **QEMU Virtual Machines Simple (VMS)** — a lightweight OpenWrt package
for running and managing QEMU virtual machines from LuCI, aimed at x86_64
routers/gateways with enough RAM and a spare CPU to spare on a VM or two.

This app is the LuCI frontend for the [`qemu-vms`](../../packages/utils/qemu-vms)
backend package (init script + UCI config). It does not work standalone —
`qemu-vms` must be installed as well (it is pulled in automatically as a
dependency).

## Requirements

- **x86_64 target only.** The backend unconditionally starts guests with
  `-enable-kvm`, and KVM acceleration only works when the guest architecture
  matches the host's — there is no cross-architecture KVM. Both packages are
  gated to `TARGET_x86_64` in their Makefiles and won't even appear in
  `menuconfig` on other targets.
- Hardware virtualization (VT-x/AMD-V) enabled in the host BIOS, `/dev/kvm`
  present.
- For PCI passthrough: IOMMU enabled in BIOS **and** on the kernel command
  line (`intel_iommu=on` / `amd_iommu=on`, plus `iommu=pt` recommended).
- Dependencies (pulled in automatically): `qemu-x86_64-softmmu`,
  `qemu-bridge-helper`, `minicom`, `socat`, `ttyd`, `usbutils`, `pciutils`.

## What it does

### Status tab
- Add / edit / delete VMs through a form (memory, vCPUs, CPU model, machine
  chipset, disk image, disk bus, optional CD-ROM, custom raw QEMU
  arguments, console type).
- Start / Stop / Restart individual VMs without touching the shell.
- Live status: running/stopped, PID, host-side RAM usage (RSS), disk image
  size — polled every few seconds.
- **Console**: opens an in-browser terminal (via `ttyd`, spawned on demand
  on an ephemeral port) attached to the VM's serial console.
- **VNC**: for VMs configured with a graphical display, opens an embedded
  [noVNC](https://github.com/novnc/noVNC) viewer over the VM's websocket
  port.
- **Log**: shows the VM's *host-side lifecycle log*
  (`/var/log/qemu-vms/<vm>.log`) — start attempts, passthrough bind
  results, configuration warnings, and QEMU's own stderr (KVM errors,
  missing image, etc.). This is deliberately separate from the guest's own
  serial console output, and is available even for VNC-mode VMs that have
  no serial console at all.

### Networks tab
Define reusable virtual network interfaces (`tap`), independent of any
one VM — the same interface definition can be attached to multiple VMs.
Each interface has:
- an optional **MAC address** (leave blank to auto-generate one with the
  `52:54:00:31:xx:xx` prefix on save),
- a **tap interface name**,
- an optional **bridge** to enslave the tap into (brought up by
  `/etc/qemu-ifup` at VM start — if no bridge is set, the interface is
  just brought up standalone),
- an emulated **network card model** (defaults to `virtio-net-pci`).

### Hardware passthrough tab
Two tables — PCI devices and USB devices — scanned live from the host
(via `lspci`/`lsusb`, falling back to raw `/sys` parsing if those tools
aren't installed).

- **Create passthrough**: creates a named `pci-passthrough` /
  `usb-passthrough` UCI section for the device (you choose the section
  name), which VMs can then reference. For PCI, this unbinds the device
  (and everything else sharing its IOMMU group) from its current driver
  and binds it to `vfio-pci`; for USB, the device is unbound from its
  kernel driver so QEMU can access it directly.
- **Remove passthrough**: only available once the device isn't attached
  to any VM. Removes the UCI section.
- The **original driver** each device was using is recorded before it's
  ever touched, so it can be restored later — see *Detaching hardware*
  below.

Devices are matched by **PCI address** (`pci_id`, e.g. `04:00.0`) for PCI,
and by **vendor:product ID** for USB (not bus/device number, which isn't
stable across reconnects/reboots — note this means two USB devices sharing
the same VID:PID can't currently be told apart).

### ToDO
##### Detaching hardware from a VM back to the host


Removing a passthrough section from LuCI only removes the *configuration*
— it does not by itself unbind the device back to its original driver.
To do that, run on the router:

```sh
/etc/init.d/qemu-vms release_pci_passthrough <pci-passthrough section name>
/etc/init.d/qemu-vms release_usb_passthrough <usb-passthrough section name>
```

This rebinds the device (and, for PCI, every device sharing its IOMMU
group) to whatever driver it was using before `qemu-vms` first touched it.

## Notes and caveats

- **Changing `disk_bus` on an existing VM does not convert the disk
  image.** A guest installed under `virtio` generally won't boot under
  `ide`/`sata` without the right drivers already present (or vice versa);
  this option is for new installs or guests that never had virtio support
  to begin with (e.g. older Windows).
- **Custom QEMU arguments** (`custom_arg`) are appended to the command
  line as literal argv entries, not re-parsed by a shell — this is
  intentional: it lets you pass arbitrary flags without risking shell
  metacharacters in that field being interpreted as commands.
- **ttyd and VNC websocket ports must be reachable from your browser**,
  same as any other port opened directly on the router — if you're behind
  a firewall/NAT/reverse-proxy that only forwards 80/443, plan
  accordingly.
- Saving VM/network/passthrough settings writes to UCI immediately but
  does **not** automatically restart the affected VM — you need to
  explicitly Restart it from the Status tab for changes to take effect.

## License

GPL-3.0 

## QEMU-VMS


### VMS - Virtual Machine Simple
Simple manager qemu virtual machines

#

### UCI configuration

All setting stored in `/etc/config/qemu-vms`
#
#### `config vm` — virtual machine instanse
|Option	| Type | Desc |	Default|
|-------|------|------|--------|
|`enabled`| boolean | Enable start | `0` |
|`image`| string | Image Disk path | – |
|`cdrom`| string	| ISO-image path | – |
|`memory`| integer | RAM Size in MB	| `128` |
|`smp`|	integer|	vCPU's|	`1`|
|`cpu`*|	string| CPU model |host|
|`machine`* |	string|	Chipset machine type|	q35
|`disk_bus`|	string|	Disk Bus |virtio
|`display_type`|	string|	Console type |serial
|`vnc_port`|	integer	|VNC-port if `display_type=vnc` |	–
|`pci_passthrough`|	list|	list pci devices passthrough|	–|
|`usb_passthrough`|	list|	list usb devices passthrough|	–|
|`network`|	list|	attached virtual network adapters | - |
|`custom_arg`|	list|	Addional QEMU cmd args |	–|

##### Example
```uci
config vm 'router'
    option enabled '1'
    option image '/path/to/disk.img'
    option memory '512'
    option smp '2'
    option cpu 'host'
    option machine 'q35'
    option disk_bus 'virtio'
    option display_type 'serial'
    list network 'example_lan'
    list pci_passthrough 'pci_example'
    list usb_passthrough 'usb_example'
    list custom_arg '-drive if=floppy,file=/mnt/disk/vm/floppy_xp.img,format=raw'
 ```
 #
#### `config network` — virtual machine network adapters
|Option	| Type | Desc |	Default|
|----------|-------|-------|------|
|`mac`|string|adapter mac-address| - |
|`bridge`|string|assign to bridge name|-|
|`ifname`**|string|iface name in host|-|
|`driver`|string|guest network adapter driver |vfio-net-pci

##### Example
```uci
config network 'example_lan'
	option iface 'tap0'
	option bridge 'br-vm'
	option driver 'pcnet'
```
#

#### `config pci_passthrough` — Passthrough PCI devices
|Option	| Type | Desc |
|----------|-------|-------|
|`pci_id`**|string|device BDF-addres| 
|`vendor_id`|string|Vendor:Device ID|

##### Example

```uci
config pci-passthrough 'pci_example'
    option pci_id '03:00.0'
    option vendor_id '8086 15f3'
 ```
#

 #### `config usb_passthrough` — Passthrough USB devices

|Option	| Type | Desc |
|----------|-------|-------|
|`product_id`**|string|Product device ID| 
|`vendor_id`**|string|Vendor Device ID|

##### Example

```uci
config usb_passthrough 'usb_example'
    option vendor_id '0bda'
    option product_id 'c811'
 ```
#
\* \-  See help qemu for list all values
\**- Mandatory options
#

### Usage init.d
* /etc/init.d/qemu-vms start|stop|restart [vm_instance]

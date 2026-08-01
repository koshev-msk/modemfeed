include $(TOPDIR)/rules.mk

LUCI_TITLE:=Web UI for QEMU Virtual Machine Simple (VMS)
LUCI_DEPENDS:=@TARGET_x86_64 +qemu-vms +ttyd +usbutils +pciutils
PKG_LICENSE:=GPLv3
PKG_VERSION:=0.0.2
PKG_RELEASE:=1

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

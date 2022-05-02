include $(TOPDIR)/rules.mk

LUCI_TITLE:=GNSS Information dashboard for 3G/LTE dongle
LUCI_DEPENDS:=+lua +curl +lua-rs232 +luasocket +iwinfo +libiwinfo-lua +lua-bit32
PKG_LICENSE:=GPLv3
PKG_VERSION:=1.2.4

define Package/luci-app-gpoint/conffiles
	/etc/config/gpoint
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

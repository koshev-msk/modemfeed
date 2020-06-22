include $(TOPDIR)/rules.mk

LUCI_TITLE:=Information dashboard for 3G/LTE dongle
LUCI_DEPENDS:=+comgt
PKG_LICENSE:=GPLv3

define Package/luci-app-modeminfo/conffiles
	/etc/config/modeminfo
endef

define Package/luci-app-modeminfo/postinst
	chmod +x /usr/bin/modeminfo
	/usr/bin/modeminfo firstinstall
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

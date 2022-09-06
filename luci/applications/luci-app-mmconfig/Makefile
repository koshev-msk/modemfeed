include $(TOPDIR)/rules.mk

LUCI_TITLE:=Configrure modem bands via mmcli utility
LUCI_DEPENDS:=+modemmanager +luci-compat
PKG_LICENSE:=GPLv3
PKG_VERSION:=0.0.5-1

define Package/luci-app-mmconfig/conffiles
	/etc/config/modemconfig
endef

define Package/luci-app-mmconfig/postrm
	rm -f /tmp/luci-indexcache
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

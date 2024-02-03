include $(TOPDIR)/rules.mk

LUCI_TITLE:=Configrure modem bands via mmcli utility
LUCI_DEPENDS:=+luci-proto-modemmanager +luci-app-modeminfo
PKG_LICENSE:=GPLv3
PKG_VERSION:=0.1.0-0

define Package/luci-app-mmconfig/postrm
	rm -f /etc/config/modemconfig
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

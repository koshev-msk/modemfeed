include $(TOPDIR)/rules.mk

LUCI_TITLE:=Web UI for smstools3
LUCI_DEPENDS:=+smstools3 +iconv +luci-compat
PKG_LICENSE:=GPLv3
PKG_VERSION:=0.0.8-3

define Package/luci-app-smstools3/postrm
	rm -f /etc/config/smstools3
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

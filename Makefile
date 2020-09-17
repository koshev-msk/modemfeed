include $(TOPDIR)/rules.mk

LUCI_TITLE:=Web UI for smstools3
LUCI_DEPENDS:=+smstools3 +iconv
PKG_LICENSE:=GPLv3

define Package/luci-app-smstools3/postrm
	rm -f /tmp/luci-indexcache
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

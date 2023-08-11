include $(TOPDIR)/rules.mk
	
LUCI_TITLE:=Web UI for atinout
LUCI_DEPENDS:=+atinout +luci-compat
PKG_LICENSE:=GPLv3
PKG_VERSION:=0.0.4
	
define Package/luci-app-atinout/postrm
	rm -rf /etc/config/atinout
endef
	
include $(TOPDIR)/feeds/luci/luci.mk
	
# call BuildPackage - OpenWrt buildroot signature

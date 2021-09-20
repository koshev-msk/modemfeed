include $(TOPDIR)/rules.mk

LUCI_TITLE:=Information dashboard for 3G/LTE dongle
LUCI_DEPENDS:=+comgt +luci-compat
PKG_LICENSE:=GPLv3
<<<<<<< HEAD
PKG_VERSION:=0.2.0
PKG_RELEASE:=beta~1
=======
PKG_VERSION:=0.1.9
PKG_RELEASE:=5
>>>>>>> 98c38c813a9b17fe66983344e5b40b4d46f3898d

define Package/luci-app-modeminfo/conffiles
	/etc/config/modeminfo
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

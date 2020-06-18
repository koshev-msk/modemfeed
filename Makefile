include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-modeminfo
PKG_VERSION:=0.0.3
PKG_RELEASE:=1

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
include $(INCLUDE_DIR)/package.mk

define Package/luci-app-modeminfo
  SECTION:=net
  SUBMENU:=Luci
  CATEGORY:=Network
  DEPENDS:=+luci +comgt
  TITLE:=Info for USB modems
  PKGARCH:=all
endef

define Package/luci-app-modeminfo/description
	Info for 3G/LTE modems
endef


define Build/Compile
endef

define Package/luci-app-modeminfo/install
	$(CP) ./files/* $(1)/
endef

define Package/luci-app-modeminfo/postinst
	ln -s /usr/share/modeminfo/cgi-bin/modeminfo.sh /usr/bin/modeminfo
	rm -rf /tmp/luci-indexcache /tmp/luci-modulecache
endef

define Package/luci-app-modeminfo/postrm
	rm -rf /tmp/luci-indexcache /tmp/luci-modulecache
	rm -rf /usr/bin/modeminfo
endef



$(eval $(call BuildPackage,luci-app-modeminfo))

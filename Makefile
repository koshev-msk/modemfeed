include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-modeminfo
PKG_VERSION:=0.0.3
PKG_RELEASE:=1

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  DEPENDS:=+luci +comgt
  TITLE:=Information dashboard for 3G/LTE dongle

  PKGARCH:=all
endef

define Package/$(PKG_NAME)/description
	LuCI information dashboard for 3G/LTE dongle
endef


define Build/Compile
endef

define Package/$(PKG_NAME)/conffiles
/etc/config/modeminfo
endef

define Package/$(PKG_NAME)/install
	$(CP) ./files/* $(1)/
endef

define Package/$(PKG_NAME)/postinst
	ln -s /usr/share/modeminfo/cgi-bin/modeminfo.sh /usr/bin/modeminfo
	rm -rf /tmp/luci-indexcache /tmp/luci-modulecache
	sh /usr/bin/modeminfo firstinstall
endef

define Package/$(PKG_NAME)/postrm
	rm -rf /tmp/luci-indexcache /tmp/luci-modulecache
	rm -rf /usr/bin/modeminfo
endef



$(eval $(call BuildPackage,$(PKG_NAME)))

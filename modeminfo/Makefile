include $(TOPDIR)/rules.mk

PKG_NAME:=modeminfo
PKG_VERSION:=0.3.0
PKG_RELEASE:=0
PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  PKGARCH:=all
  DEPENDS:=
  TITLE:=Signal information for 3G/LTE dongle
endef

define Package/$(PKG_NAME)-serial
  PKGARCH:=all
  DEPENDS=+comgt +atinout +modeminfo
  TITLE:=Signal information for 3G/LTE dongle. Serial addon
endef

define Package/$(PKG_NAME)-qmi
  PKGARCH:=all
  DEPENDS=+qmi-utils +modeminfo
  TITLE:=Signal information for 3G/LTE dongle. QMI addon
endef

define Package/$(PKG_NAME)/description
	Signal information for 3G/LTE dongle
endef

define Package/$(PKG_NAME)-serial/description
        Serial addon for modeminfo
endef

define Package/$(PKG_NAME)/description
        QMI addon for modeminfo
endef

define Package/$(PKG_NAME)/conffiles
	/etc/config/modeminfo
endef


define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef


define Package/$(PKG_NAME)/install
	$(CP) ./root/usr/bin/* $(1)/usr/bin
endef

define Package/$(PKG_NAME)-serial/install
        $(CP) ./root/usr/share/modeminfo/* $(1)/usr/share/modeminfo
endef

define Package/$(PKG_NAME)-serial/install
        $(CP) ./root/usr/share/modeminfo-qmi/* $(1)/usr/share/modeminfo
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)-serial))
$(eval $(call BuildPackage,$(PKG_NAME)-qmi))

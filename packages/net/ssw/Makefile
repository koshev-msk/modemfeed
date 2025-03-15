include $(TOPDIR)/rules.mk

PKG_NAME:=ssw
PKG_VERSION:=0.0.2
PKG_RELEASE:=1
PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  PKGARCH:=all
  DEPENDS:=+modemmanager +mwan3
  TITLE:=SIM-card switcher and tracker
endef

define Package/$(PKG_NAME)/description
	SIM-card tracker for HuastLink HC-G60 and ZBTLint ZBT-WE2806-A
	or similar CPE-routers with two SIM slots and one 4g/5G modem
endef


define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef


define Package/$(PKG_NAME)/install
	$(CP) ./root/* $(1)/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

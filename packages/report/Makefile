
include $(TOPDIR)/rules.mk

PKG_NAME:=report
PKG_VERSION:=1
PKG_RELEASE:=0
PKG_BUILD_DIR:=$(BUILD_DIR)/report

include $(INCLUDE_DIR)/package.mk

define Package/report
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Generate report
  DEPENDS:=
endef

define Package/report/description
	 Utility to generate report
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/report/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) ./files/report $(1)/bin/
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/report.config $(1)/etc/config/report
endef

$(eval $(call BuildPackage,report))

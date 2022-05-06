
include $(TOPDIR)/rules.mk

PKG_NAME:=iolines
PKG_VERSION:=1
PKG_RELEASE:=0
PKG_BUILD_DIR:=$(BUILD_DIR)/iolines

include $(INCLUDE_DIR)/package.mk

define Package/iolines
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Network interface watchdog
  DEPENDS:=
endef

define Package/iolines/description
	 This package contains a utility for managing the interface using ping requests.
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/iolines/install
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/iolines.config $(1)/etc/config/iolines
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/iolines.init $(1)/etc/init.d/iolines
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./files/iolines.defaults $(1)/etc/uci-defaults/20_iolines
endef

$(eval $(call BuildPackage,iolines))

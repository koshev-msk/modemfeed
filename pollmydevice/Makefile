
include $(TOPDIR)/rules.mk

PKG_NAME:=pollmydevice
PKG_VERSION:=1
PKG_RELEASE:=8
PKG_BUILD_DIR:=$(BUILD_DIR)/pollmydevice

PKG_MAINTAINER:=TELEOFIS

include $(INCLUDE_DIR)/package.mk

define Package/pollmydevice
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=PollMyDevice project
  DEPENDS:=+libpthread +libuci
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib

define Package/pollmydevice/description
 This package contains utility for sim check manage.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/pollmydevice/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pollmydevice $(1)/sbin/
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/pollmydevice.config $(1)/etc/config/pollmydevice
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/pollmydevice.init $(1)/etc/init.d/pollmydevice
	$(INSTALL_DIR) $(1)/etc/pollmydevice
	$(CP) ./files/console $(1)/etc/pollmydevice/console
endef

$(eval $(call BuildPackage,pollmydevice))

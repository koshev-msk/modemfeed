include $(TOPDIR)/rules.mk

PKG_NAME:=microdns
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=IP Addresses and Names
  TITLE:=Very simple DNS forwarder
  MAINTAINER:=Konstantine Shevlakov
endef

define Package/$(PKG_NAME)/description
 This package forward upstream dns from resolv.conf
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS)  -Wextra -pthread -Wall \
		-o $(PKG_BUILD_DIR)/$(PKG_NAME) $(PKG_BUILD_DIR)/$(PKG_NAME).c
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

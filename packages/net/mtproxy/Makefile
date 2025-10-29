include $(TOPDIR)/rules.mk

PKG_NAME:=mtproxy
PKG_VERSION:=3.0.6
PKG_RELEASE:=1

PKG_MAINTAINER:=Kosntantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=LICENSE

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/GetPageSpeed/MTProxy.git
PKG_SOURCE_VERSION:=d5348375088d1566707c061fadd3be753d77034a

PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SUBMENU:=Web Servers/Proxies
  SECTION:=net
  CATEGORY:=Network
  TITLE:=MTProxy OpenWRT package
  DEPENDS:=+libopenssl +zlib
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/objs/bin/* \
			$(1)/usr/bin/
endef

ifeq ($(ARCH),$(filter $(ARCH), x86_64))
  $(eval $(call BuildPackage,$(PKG_NAME)))
endif

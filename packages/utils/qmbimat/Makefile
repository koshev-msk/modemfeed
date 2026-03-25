include $(TOPDIR)/rules.mk

PKG_NAME:=qmbimat
PKG_VERSION:=0.0.1
PKG_RELEASE:=1

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=LICENSE
PKG_LICENSE_FILES:=COPYING

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/quectel-official/QMbimAT.git
PKG_SOURCE_VERSION:=7c665521a732acbe7722ed19c91011163649f899

PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Network
  TITLE:=Tool for send AT-command via MBIM interface
  URL:=https://github.com/quectel-official/QMbimAT
endef

define Package/$(PKG_NAME)/description
	Tool for send AT-command via MBIM interface
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

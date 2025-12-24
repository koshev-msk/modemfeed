include $(TOPDIR)/rules.mk

PKG_NAME:=flash_fox
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=LICENSE
PKG_LICENSE_FILES:=COPYING

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/andrewz1/flash_fox.git
PKG_SOURCE_VERSION:=417257e1d95724380097a72380b89e36f75e3598

PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)


include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  MAINTAINER := Andrei Safonov <andrey.safonov@gmail.com>
  TITLE:=Tool for flashing QUalcomm T99W175 modems
  URL:=https://github.com/andrewz1/flash_fox/
  DEPENDS:=+libusb-1.0 +zlib
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

include $(TOPDIR)/rules.mk

PKG_NAME:=teleproxy
PKG_VERSION:=4.15.0
PKG_RELEASE:=1

PKG_MAINTAINER:=Kosntantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=LICENSE

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/teleproxy/teleproxy.git
PKG_SOURCE_VERSION:=1c989687ef3f163026c102bb5a2eed497e8611ba

PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)

include $(INCLUDE_DIR)/package.mk

MAKE_VARS += EXTRA_VERSION=$(PKG_VERSION)

define Package/$(PKG_NAME)
  SUBMENU:=Web Servers/Proxies
  SECTION:=net
  CATEGORY:=Network
  TITLE:=MTProxy OpenWRT package
  DEPENDS:=+libopenssl +zlib
endef

define Package/$(PKG_NAME)/conffiles
/etc/config/$(PKG_NAME)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin \
			$(1)/usr/share \
			$(1)/etc/config \
			$(1)/etc/init.d \
			$(1)/etc/uci-defaults
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/objs/bin/* \
			$(1)/usr/bin/
	$(INSTALL_BIN) ./files/$(PKG_NAME).init $(1)/etc/init.d/$(PKG_NAME)
	$(INSTALL_BIN) ./files/$(PKG_NAME).defaults  $(1)/etc/uci-defaults/$(PKG_NAME)
	$(INSTALL_BIN) ./files/$(PKG_NAME)-config-refresh.sh $(1)/usr/share/
	$(INSTALL_CONF) ./files/$(PKG_NAME).config $(1)/etc/config/$(PKG_NAME)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

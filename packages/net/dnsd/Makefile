include $(TOPDIR)/rules.mk

PKG_NAME:=dnsd
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/koshev-msk/dnsd.git
PKG_SOURCE_DATE:=2026-11-26
PKG_SOURCE_VERSION:=3a59f20d26663be7454c559e79696219400d93e7
PKG_MIRROR_HASH:=skip

PKG_MAINTAINER:=Konstantine <shevlakov@132lan.ru>
PKG_LICENSE:=GPL-3.0-or-later
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=net
	CATEGORY:=Network
	SUBMENU:=IP Addresses and Names
	TITLE:=Lightweight DNS-over-HTTPS proxy
	URL:=https://github.com/kamarya/dnsd
	DEPENDS:=+libcurl +ca-bundle
endef

define Package/$(PKG_NAME)/description
	Local UDP DNS forwarder that sends queries to an RFC 8484 DoH server.
	No cache, one process, libcurl does the TLS/HTTP part.
endef

# upstream Makefile ignores CFLAGS/LDFLAGS, so call the compiler directly
define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -std=gnu11 -Wall \
		-I$(PKG_BUILD_DIR)/inc $(PKG_BUILD_DIR)/src/dnssec.c \
		$(TARGET_LDFLAGS) -lcurl -o $(PKG_BUILD_DIR)/$(PKG_NAME)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

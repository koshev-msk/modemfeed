include $(TOPDIR)/rules.mk

PKG_NAME:=vless-core-cli
PKG_VERSION:=1.1.4
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/notfence/vless-core-cli/tar.gz/v$(PKG_VERSION)?
PKG_HASH:=c7b1e6ce59416b3511e8e09049f1dd44927bc355f81b2cd113f4f9c1aaba5cbb

PKG_MAINTAINER:=Konstantine Shevlakov <kosnt.shell@gmail.com>
PKG_LICENSE_FILES:=THIRD_PARTY_LICENSES.txt

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/vless-core-cli
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Web Servers/Proxies
  TITLE:=Standalone C VLESS client with local SOCKS5
  URL:=https://github.com/notfence/vless-core-cli
  DEPENDS:=+libopenssl
endef

define Package/vless-core-cli/description
  Standalone C client for VLESS (TCP/XHTTP/gRPC/WS, Reality/TLS) and SOCKS5
  upstreams. Listens on a local SOCKS5 port on 127.0.0.1.
endef

define Package/vless-core-cli/conffiles
/etc/config/vless-core
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) linux \
		CC="$(TARGET_CC)" \
		COMMON_CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -std=c11 -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		BIN_LINUX=vless-core
endef

define Package/vless-core-cli/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/etc/init.d $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vless-core $(1)/usr/bin/vless-core
	$(INSTALL_BIN) ./files/vless-core.init $(1)/etc/init.d/vless-core
	$(INSTALL_CONF) ./files/vless-core.config $(1)/etc/config/vless-core
endef

$(eval $(call BuildPackage,vless-core-cli))

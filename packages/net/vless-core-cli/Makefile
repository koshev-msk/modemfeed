include $(TOPDIR)/rules.mk

PKG_NAME:=vless-core-cli
PKG_VERSION:=2.0.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/notfence/vless-core-cli/tar.gz/v$(PKG_VERSION)?
PKG_HASH:=d9212ce3955f7824fa9113c34df14570f52a6ad62f47e7e2a7d1a5dbd13cada4

PKG_MAINTAINER:=Konstantine Shevlakov <kosnt.shell@gmail.com>
PKG_LICENSE_FILES:=THIRD_PARTY_LICENSES.txt

PKG_BUILD_FLAGS:=gc-sections

include $(INCLUDE_DIR)/package.mk

define Package/vless-core-cli
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Web Servers/Proxies
  TITLE:=Standalone C proxy client (VLESS/Trojan/VMess/SS) with local SOCKS5
  URL:=https://github.com/notfence/vless-core-cli
  DEPENDS:=+libopenssl
endef

define Package/vless-core-cli/description
  Standalone C client for VLESS, Trojan, VMess, Shadowsocks and SOCKS5
  upstreams (TCP/WS/gRPC/XHTTP/HTTPUpgrade, TLS/Reality).
  Listens on a local SOCKS5 port on 127.0.0.1.
endef

define Package/vless-core-cli/conffiles
/etc/config/vless-core
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -std=c11 -Wall \
		-I$(PKG_BUILD_DIR)/include $(PKG_BUILD_DIR)/src/*.c \
		-o $(PKG_BUILD_DIR)/vless-core \
		$(TARGET_LDFLAGS) -lssl -lcrypto -lpthread
endef

define Package/vless-core-cli/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/etc/init.d $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vless-core $(1)/usr/bin/vless-core
	$(INSTALL_BIN) ./files/vless-core.init $(1)/etc/init.d/vless-core
	$(INSTALL_CONF) ./files/vless-core.config $(1)/etc/config/vless-core
endef

$(eval $(call BuildPackage,vless-core-cli))

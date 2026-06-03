include $(TOPDIR)/rules.mk

PKG_NAME:=opera-proxy
PKG_VERSION:=1.22.0
PKG_RELEASE:=1

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=MIT

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

# --- Маппинг OpenWrt ARCH+CPU → имя бинарника на GitHub ---
# Формат имён на GitHub: opera-proxy.linux-<suffix>
#
# OpenWrt $(ARCH) / $(CONFIG_CPU_TYPE)  →  суффикс GitHub
#   mips     / 24kc                     →  mips
#   mipsel   / 24kc                     →  mipsle
#   aarch64  / cortex-a53 и др.         →  arm64
#   arm      / cortex-a7 и др.          →  arm
#   arm      / cortex-a9+vfpv3          →  arm  (armv6 нет, ближайший arm)
#   x86_64   / -                        →  amd64
#   i386     / -                        →  386

ifeq ($(ARCH),mips)
  GOARCH_SUFFIX:=mips
else ifeq ($(ARCH),mipsel)
  GOARCH_SUFFIX:=mipsle
else ifeq ($(ARCH),mips64)
  GOARCH_SUFFIX:=mips64
else ifeq ($(ARCH),mips64el)
  GOARCH_SUFFIX:=mips64le
else ifeq ($(ARCH),aarch64)
  GOARCH_SUFFIX:=arm64
else ifeq ($(ARCH),arm)
  GOARCH_SUFFIX:=arm
else ifeq ($(ARCH),x86_64)
  GOARCH_SUFFIX:=amd64
else ifeq ($(ARCH),i386)
  GOARCH_SUFFIX:=386
else
  GOARCH_SUFFIX:=$(ARCH)
endif

PKG_SOURCE_URL:=https://github.com/Alexey71/opera-proxy/releases/download/v$(PKG_VERSION)
PKG_SOURCE:=opera-proxy.linux-$(GOARCH_SUFFIX)
PKG_HASH:=skip

include $(INCLUDE_DIR)/package.mk

define Package/opera-proxy
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Standalone Opera VPN proxy client
  URL:=https://github.com/Alexey71/opera-proxy
  DEPENDS:=
endef

define Package/opera-proxy/description
  Standalone Opera VPN client. Starts a plain HTTP/SOCKS5 proxy server
  forwarding traffic through Opera VPN proxies.
  Supports country selection: EU, AS, AM.
endef

define Package/opera-proxy/conffiles
/etc/config/opera-proxy
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(DL_DIR)/$(PKG_SOURCE) $(PKG_BUILD_DIR)/opera-proxy
	chmod +x $(PKG_BUILD_DIR)/opera-proxy
endef

define Build/Compile
endef

define Package/opera-proxy/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/opera-proxy $(1)/usr/bin/opera-proxy

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/files/opera-proxy.init $(1)/etc/init.d/opera-proxy

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(CURDIR)/files/opera-proxy.config $(1)/etc/config/opera-proxy
endef

$(eval $(call BuildPackage,opera-proxy))

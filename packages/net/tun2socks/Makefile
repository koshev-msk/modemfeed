include $(TOPDIR)/rules.mk

PKG_NAME:=tun2socks
PKG_VERSION:=2.7.0
PKG_RELEASE:=2

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/xjasonlyu/tun2socks.git
PKG_SOURCE_VERSION:=8dda19e8e4613e014f0b12f3e624fdff5e5f23b3

PKG_MAINTAINER:=Konstantine Shevlakov <shevlako@132lan.ru>
PKG_LICENSE:=GPL-3.0
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=golang/host upx/host
PKG_BUILD_PARALLEL:=1
PKG_BUILD_FLAGS:=no-mips16

GO_PKG=github.com/xjasonlyu/tun2socks

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/packages/lang/golang/golang-package.mk

# --- UPX support ---
UPX:=$(STAGING_DIR_HOST)/bin/upx
UPX_AVAILABLE:=$(shell [ -x "$(UPX)" ] && echo yes || echo no)

ifeq ($(UPX_AVAILABLE),yes)
define upxcompress
	@echo "Compressing $(1) with UPX..."
	$(UPX) --lzma --best $(1)
endef
else
define upxcompress
	@echo "UPX not found at $(UPX), skipping compression"
endef
endif

GO_PKG_LDFLAGS:=-s -w
GO_PKG_LDFLAGS_X:= \
	github.com/xjasonlyu/tun2socks/internal/version.Version=v$(PKG_VERSION) \
	github.com/xjasonlyu/tun2socks/internal/version.GitCommit=$(shell echo $(PKG_SOURCE_VERSION) | cut -c1-7)
GO_MOD_ARGS:=
GO_PKG_BUILD_VARS+= GO111MODULE=on

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  TITLE:=tun to socks proxy redirector written on golang
  URL:=https://github.com/xjasonlyu/tun2socks
  DEPENDS:=$(GO_ARCH_DEPENDS) +ca-bundle \
	   +kmod-tun +ip-full +coreutils-base64
endef

define Package/$(PKG_NAME)/install
	$(call GoPackage/Package/Install/Bin,$(PKG_INSTALL_DIR))
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/$(PKG_NAME) $(1)/usr/sbin/
	$(call upxcompress,$(1)/usr/sbin/$(PKG_NAME))
	$(CP) ./root/* $(1)/
endef

$(eval $(call GoBinPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)))

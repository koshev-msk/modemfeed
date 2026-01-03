# Makefile build Torrserver MatriX for OpenWrt SDK
# add node repository in feeds.conf.default for build OpenWrt current branch: e.g 24.10
# src-git node https://github.com/nxhack/openwrt-node-packages.git;openwrt-24.10
# and optional for compress binary via upx
# src-git upx https://github.com/kuoruan/openwrt-upx.git
# Copyright (c) Konstantine Shevlakov 2025 at <shevlakov@132lan.ru>

include $(TOPDIR)/rules.mk

PKG_NAME:=torrserver
PKG_VERSION:=137
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/YouROK/TorrServer.git
PKG_SOURCE_VERSION:=19e4f4ef78ad92e68a19fafc561003ca33e0dea8
PKG_MIRROR_HASH:=skip

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=golang/host node/host node-yarn/host upx/host
PKG_BUILD_PARALLEL:=1
PKG_BUILD_FLAGS:=no-mips16

include $(INCLUDE_DIR)/package.mk

GO_ARCH:=
ifeq ($(ARCH),aarch64)
  GO_ARCH:=arm64
else ifeq ($(ARCH),arm)
  GO_ARCH:=arm
else ifeq ($(ARCH),i386)
  GO_ARCH:=386
else ifeq ($(ARCH),i686)
  GO_ARCH:=386
else ifeq ($(ARCH),x86_64)
  GO_ARCH:=amd64
else ifeq ($(ARCH),mips)
  GO_ARCH:=mips
else ifeq ($(ARCH),mipsel)
  GO_ARCH:=mipsle
else ifeq ($(ARCH),mips64)
  GO_ARCH:=mips64
else ifeq ($(ARCH),mips64el)
  GO_ARCH:=mips64le
else ifeq ($(ARCH),loongarch64)
  GO_ARCH:=loong64
else ifeq ($(ARCH),riscv64)
  GO_ARCH:=riscv64
else ifeq ($(ARCH),riscv32)
  GO_ARCH:=riscv
else ifeq ($(ARCH),powerpc)
  GO_ARCH:=ppc
else ifeq ($(ARCH),powerpc64)
  GO_ARCH:=ppc64
else ifeq ($(ARCH),powerpc64le)
  GO_ARCH:=ppc64le
else
  GO_ARCH:=$(ARCH)
endif

define Package/$(PKG_NAME)
  SECTION:=multimedia
  CATEGORY:=Multimedia
  TITLE:=TorrServer - stream torrent files
  URL:=https://github.com/YouROK/TorrServer
  DEPENDS:=$(GO_ARCH_DEPENDS) +ca-bundle
endef

define Package/$(PKG_NAME)/description
  TorrServer is a tool for streaming torrent files.
endef

GO_HOST:=$(STAGING_DIR_HOSTPKG)/bin/go
UPX:=$(STAGING_DIR_HOST)/bin/upx
UPX_AVAILABLE:=$(shell [ -x "$(UPX)" ] && echo yes || echo no)
TARGET_CC:=$(TARGET_CROSS)gcc
TARGET_CXX:=$(TARGET_CROSS)g++


ifeq ($(UPX_AVAILABLE),yes)
define upxcompress
	@echo "Compressing $(PKG_NAME) with UPX..."
	$(UPX) --lzma --best $(1)
endef
else
define upxcompress
	@echo "UPX not found at $(UPX)"
	@echo "Skipping compression of $(PKG_NAME)"
endef
endif

define Build/Prepare
	$(call Build/Prepare/Default)

	cd $(PKG_BUILD_DIR) && \
	ln -sf server/go.mod go.mod && \
	ln -sf server/go.sum go.sum
endef

define Build/Compile
	# Build Application WEB Interface
	cd $(PKG_BUILD_DIR) && \
	export PATH=$(STAGING_DIR_HOSTPKG)/bin:$$PATH && \
	NODE_OPTIONS=--openssl-legacy-provider \
	$(GO_HOST) run gen_web.go

	# Prepare Build Application
	cd $(PKG_BUILD_DIR)/server && \
	$(GO_HOST) clean -i -r -cache && \
	$(GO_HOST) mod tidy

	# Build main Application
	cd $(PKG_BUILD_DIR)/server && \
	GOOS=linux \
	GOARCH=$(GO_ARCH) \
	$(if $(filter arm,$(ARCH)),GOARM=$(if $(filter bcm53xx kirkwood,$(BOARD)),5,7)) \
	$(if $(filter mips mipsel,$(ARCH)),GOMIPS=softfloat) \
	$(if $(filter mips64 mips64el,$(ARCH)),GOMIPS64=softfloat) \
	CGO_ENABLED=0 \
	$(GO_HOST) build \
		-trimpath \
		-ldflags="-s -w -checklinkname=0 -extldflags=-static" \
		-tags=nosqlite \
		-o $(PKG_INSTALL_DIR)/$(PKG_NAME) \
		./cmd
	$(call upxcompress,$(PKG_INSTALL_DIR)/$(PKG_NAME))
endef

define Package/$(PKG_NAME)/install	
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/$(PKG_NAME) $(1)/usr/bin/
	
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/torrserver.config $(1)/etc/config/torrserver
	
	$(INSTALL_DIR) $(1)/usr/share/torrserver
	
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/torrserver.init $(1)/etc/init.d/torrserver
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

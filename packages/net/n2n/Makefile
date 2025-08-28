#
# Copyright (C) 2021 - ntop.org and contributors
#

include $(TOPDIR)/rules.mk

PKG_NAME:=n2n
PKG_VERSION:=3.1.1
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

# These are defaults for compiling without any environmental overrides
# (eg, the github action calculates the correct overrides for each build)
PKG_SOURCE_URL:=https://github.com/ntop/n2n
PKG_SOURCE_VERSION:=dev
PKG_MIRROR_HASH:=skip

# Apply overrides from the build environment
ifdef N2N_PKG_SOURCE_URL
    PKG_SOURCE_URL:=$(N2N_PKG_SOURCE_URL)
endif
ifdef N2N_PKG_SOURCE_VERSION
    PKG_SOURCE_VERSION:=$(N2N_PKG_SOURCE_VERSION)
endif
ifdef N2N_PKG_VERSION
    PKG_VERSION:=$(N2N_PKG_VERSION)
endif

PKG_MAINTAINER:=Emanuele Faranda <faranda@ntop.org>
PKG_LICENSE:=GPL3

# autogen fix
PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=N2N Peer-to-peer VPN
  URL:=http://www.ntop.org/n2n
  SUBMENU:=VPN
  DEPENDS+=+libcap
endef

define Package/n2n-edge
  $(call Package/n2n/Default)
  TITLE+= client (edge node)
  DEPENDS+=+kmod-tun
endef

define Package/n2n-supernode
  $(call Package/n2n/Default)
  TITLE+= server (supernode)
endef

define Package/$(PKG_NAME)-tools
  $(call Package/n2n/Default)
  TITLE+= tools
endef

define Package/$(PKG_NAME)-tests
  $(call Package/n2n/Default)
  TITLE+= benchmark
endef

define Package/$(PKG_NAME)-edge/description
The client node for the N2N infrastructure
endef

define Package/$(PKG_NAME)-supernode/description
The supernode for the N2N infrastructure
endef

define Package/$(PKG_NAME)-tools/description
Peer-to-peer N2N VPN, keygen, keyfile, sertificate creation tools
endef

define Package/$(PKG_NAME)-tests/description
Peer-to-peer N2N VPN  benchmarks
endef


define Build/Configure
	( cd $(PKG_BUILD_DIR); \
	./autogen.sh; \
	./configure )
endef

define Package/n2n-edge/conffiles
/etc/n2n/edge.conf
endef

define Package/n2n-edge/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/edge $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/packages/openwrt/etc/init.d/edge $(1)/etc/init.d/edge
	$(INSTALL_DIR) $(1)/etc/n2n
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/packages/etc/n2n/edge.conf.sample $(1)/etc/n2n/edge.conf
endef

define Package/n2n-supernode/conffiles
/etc/n2n/supernode.conf
endef

define Package/n2n-supernode/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/supernode $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/packages/openwrt/etc/init.d/supernode $(1)/etc/init.d/supernode
	$(INSTALL_DIR) $(1)/etc/n2n
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/packages/etc/n2n/supernode.conf.sample $(1)/etc/n2n/supernode.conf
endef

define Package/$(PKG_NAME)-tools/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/n2n-benchmark \
		$(PKG_BUILD_DIR)/tools/n2n-decode \
		$(PKG_BUILD_DIR)/tools/n2n-keygen \
		$(PKG_BUILD_DIR)/tools/n2n-portfwd \
		$(PKG_BUILD_DIR)/tools/n2n-route \
		$(1)/usr/bin/
endef

define Package/$(PKG_NAME)-tests/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/tests-auth \
		$(PKG_BUILD_DIR)/tools/tests-compress \
		$(PKG_BUILD_DIR)/tools/tests-elliptic \
		$(PKG_BUILD_DIR)/tools/tests-hashing \
		$(PKG_BUILD_DIR)/tools/tests-transform \
		$(PKG_BUILD_DIR)/tools/tests-wire \
		$(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)-supernode))
$(eval $(call BuildPackage,$(PKG_NAME)-edge))
$(eval $(call BuildPackage,$(PKG_NAME)-tools))
$(eval $(call BuildPackage,$(PKG_NAME)-tests))

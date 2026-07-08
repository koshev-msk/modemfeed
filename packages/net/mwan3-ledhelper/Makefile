include $(TOPDIR)/rules.mk

PKG_NAME:=mwan3-ledhelper
PKG_VERSION:=0.0.2
PKG_RELEASE:=2
PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>

include $(INCLUDE_DIR)/package.mk


define Package/$(PKG_NAME)/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=WAN
  TITLE:=MWAN3 Led Helper
  PKGARCH:=all
endef

define Package/$(PKG_NAME)
  $(call Package/$(PKG_NAME)/Default)
  DEPENDS:=+mwan3
endef

define Package/$(PKG_NAME)-nft
  $(call Package/$(PKG_NAME)/Default)
  TITLE+= (nft variant)
  DEPENDS:=+mwan3-nft
endef

define Package/$(PKG_NAME)/description
	Hotplug script to indicate led for mwan3 link states
endef

define Package/$(PKG_NAME)-nft/description
	Hotplug script to indicate led for mwan3 link states (nftables/fw4 variant).
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef


define Package/$(PKG_NAME)/install
	$(CP) ./root/* $(1)/
endef

define Package/$(PKG_NAME)-nft/install
        $(CP) ./root/* $(1)/
endef

ifneq ($(MWAN3_LEDHELPER_BUILD_NFT_ONLY),1)
  $(eval $(call BuildPackage,$(PKG_NAME)))
endif
ifneq ($(MWAN3_LEDHELPER_BUILD_IPT_ONLY),1)
  $(eval $(call BuildPackage,$(PKG_NAME)-nft))
endif


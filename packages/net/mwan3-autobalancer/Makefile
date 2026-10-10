include $(TOPDIR)/rules.mk

PKG_NAME:=mwan3-autobalancer
PKG_VERSION:=0.0.1
PKG_RELEASE:=1
PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>

include $(INCLUDE_DIR)/package.mk


define Package/$(PKG_NAME)/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=WAN
  TITLE:=MWAN3 Policy Autobalancer
  PKGARCH:=all
endef

define Package/$(PKG_NAME)-nft
  $(call Package/$(PKG_NAME)/Default)
  TITLE+= (nft variant)
  DEPENDS:=+mwan3-nft
endef

define Package/$(PKG_NAME)-nft/description
	MWAN3 adaptive weights from mwan3track data (nftables)
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef


define Package/$(PKG_NAME)-nft/install
        $(CP) ./root/* $(1)/
endef

$(eval $(call BuildPackage,$(PKG_NAME)-nft))



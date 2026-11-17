include $(TOPDIR)/rules.mk

PKG_NAME:=ifb-shaper
PKG_VERSION:=0.1.1
PKG_RELEASE:=2

include $(INCLUDE_DIR)/package.mk

define Package/ifb-shaper
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Per-client HTB traffic shaper (IFB)
  DEPENDS:=+tc +ip +kmod-ifb
  PKGARCH:=all
endef

define Package/ifb-shaper/description
  Per-client upload/download shaper. Download is shaped by HTB on the
  LAN device egress, upload by HTB on an IFB device fed from the LAN
  device ingress. Configured via /etc/config/ifb-shaper.
endef

define Package/ifb-shaper/conffiles
/etc/config/ifb-shaper
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/ifb-shaper/install
	$(INSTALL_DIR) $(1)/etc/init.d $(1)/etc/config
	$(INSTALL_BIN) ./files/ifb-shaper.init $(1)/etc/init.d/ifb-shaper
	$(INSTALL_CONF) ./files/ifb-shaper.config $(1)/etc/config/ifb-shaper
endef

$(eval $(call BuildPackage,ifb-shaper))

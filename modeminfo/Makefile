include $(TOPDIR)/rules.mk

PKG_NAME:=modeminfo
PKG_VERSION:=0.3.0
PKG_RELEASE:=0
PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  PKGARCH:=all
  DEPENDS=+comgt
  TITLE:=Signal information for 3G/LTE dongle
endef

define Package/$(PKG_NAME)-serial-simcom
  PKGARCH:=all
  DEPENDS=+atinout +modeminfo
  TITLE:=SIMCOM serial addon for modeminfo
endef

define Package/$(PKG_NAME)-serial-huawei
  PKGARCH:=all
  DEPENDS=+atinout +modeminfo
  TITLE:=HUAWEI serial addon for modeminfo
endef

define Package/$(PKG_NAME)-serial-quectel
  PKGARCH:=all
  DEPENDS=+atinout +modeminfo
  TITLE:=QUECTEL serial addon for modeminfo
endef

define Package/$(PKG_NAME)-serial-fibocom
  PKGARCH:=all
  DEPENDS=+atinout +modeminfo
  TITLE:=FIBOCOM NL668\678 serial addon for modeminfo
endef

define Package/$(PKG_NAME)-qmi
  PKGARCH:=all
  DEPENDS=+qmi-utils +modeminfo
  TITLE:=QMI addon for modeminfo
endef

define Package/$(PKG_NAME)/description
	Signal information for 3G/LTE dongle
endef

define Package/$(PKG_NAME)-serial-simcom/description
        SIMCOM serial addon for modeminfo
endef

define Package/$(PKG_NAME)-serial-huawei/description
        Huawei modems serial addon for modeminfo
endef

define Package/$(PKG_NAME)-serial-quectel/description
        QUECTEL modems serial addon for modeminfo
endef

define Package/$(PKG_NAME)-serial-fibocom/description
        Fibocom NL668\NL678 serial addon for modeminfo. 
endef


define Package/$(PKG_NAME)-qmi/description
        QMI addon for modeminfo
endef

define Package/$(PKG_NAME)/conffiles
	/etc/config/modeminfo
endef


define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/share/modeminfo
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./root/usr/bin/* $(1)/usr/bin
	$(CP) ./root/usr/share/modeminfo/scripts \
		./root/usr/share/modeminfo/scripts/*.gcom \
		./root/usr/share/modeminfog/scripts/ch_to_band \
		$(1)/usr/share/modeminfo/scripts/
	$(CP) ./root/etc/config/* $(1)/etc/config
endef

define Package/$(PKG_NAME)-serial-simcom/install
		$(INSTALL_DIR) $(1)/usr/share/modeminfo/scripts
        $(CP) ./root/usr/share/modeminfo/scripts/SIMCOM* $(1)/usr/share/modeminfo/scripts
endef

define Package/$(PKG_NAME)-serial-huawei/install
		$(INSTALL_DIR) $(1)/usr/share/modeminfo/scripts
        $(CP) ./root/usr/share/modeminfo/scripts/HUAWEI* $(1)/usr/share/modeminfo/scripts
endef

define Package/$(PKG_NAME)-serial-quectel/install
		$(INSTALL_DIR) $(1)/usr/share/modeminfo/scripts
        $(CP) ./root/usr/share/modeminfo/scripts/QUECTEL* $(1)/usr/share/modeminfo/scripts
endef

define Package/$(PKG_NAME)-serial-fibocom/install
		$(INSTALL_DIR) $(1)/usr/share/modeminfo/scripts
        $(CP) ./root/usr/share/modeminfo/scripts/FIBOCOM* $(1)/usr/share/modeminfo/scripts
endef

define Package/$(PKG_NAME)-qmi/install
		$(INSTALL_DIR) $(1)/usr/share/modeminfo/scripts
        $(CP) ./root/usr/share/modeminfo-qmi/* $(1)/usr/share/modeminfo/scripts
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)-serial-simcom))
$(eval $(call BuildPackage,$(PKG_NAME)-serial-huawei))
$(eval $(call BuildPackage,$(PKG_NAME)-serial-quectel))
$(eval $(call BuildPackage,$(PKG_NAME)-serial-fibocom))
$(eval $(call BuildPackage,$(PKG_NAME)-qmi))

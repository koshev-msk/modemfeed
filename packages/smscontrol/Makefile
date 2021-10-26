
include $(TOPDIR)/rules.mk

PKG_NAME:=smscontrol
PKG_VERSION:=1
PKG_RELEASE:=0
PKG_BUILD_DIR:=$(BUILD_DIR)/smscontrol

include $(INCLUDE_DIR)/package.mk

define Package/smscontrol
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=CLI commands over sms and calls
  DEPENDS:=smstools3
endef

define Package/smscontrol/description
	 Utility to send CLI commands over sms and calls
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/smscontrol/install
	$(INSTALL_DIR) $(1)/etc/smscontrol
	$(CP) ./files/smshandler $(1)/etc/smscontrol/smshandler
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/smscontrol.config $(1)/etc/config/smscontrol
	$(CP) ./files/event_log.config $(1)/etc/config/event_log
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/smscontrol.init $(1)/etc/init.d/smscontrol
endef

$(eval $(call BuildPackage,smscontrol))

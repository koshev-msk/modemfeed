
include $(TOPDIR)/rules.mk

PKG_NAME:=simman2
PKG_VERSION:=3
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/simman2
#CMAKE_INSTALL:=1

PKG_MAINTAINER:=Konstantin Sklyarec <atskyua@gmail.com> Vladimir Ovseychuk <vgovseychuk@gmail.com>

include $(INCLUDE_DIR)/package.mk
#include $(INCLUDE_DIR)/cmake.mk

define Package/simman2
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Manager for SIM card in 3g modem 
  DEPENDS:=+libuci 
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib

define Package/simman2/description
 This package contains utility for sim check manage.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/simman2/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/simman2 $(1)/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/simman2_cli $(1)/sbin/
	$(INSTALL_DIR) $(1)//usr/libexec/rpcd
	$(INSTALL_BIN) ./files/simman2.rpcd $(1)/usr/libexec/rpcd/simman2
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/simman2.config $(1)/etc/config/simman2
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/simman2.init $(1)/etc/init.d/simman2
	$(INSTALL_DIR) $(1)/etc/simman2
	$(INSTALL_BIN) ./files/nbinfo.sh $(1)/etc/simman2/nbinfo.sh
	$(INSTALL_BIN) ./files/setsim.sh $(1)/etc/simman2/setsim.sh
	$(INSTALL_BIN) ./files/check_prio.sh $(1)/etc/simman2/check_prio.sh
	$(CP) ./files/*.gcom $(1)/etc/simman2
	$(INSTALL_DIR) $(1)/etc/hotplug.d/usb
	$(INSTALL_DATA) ./files/simman2.hotplug $(1)/etc/hotplug.d/usb/12-modem
	$(INSTALL_DATA) ./files/scmux.hotplug $(1)/etc/hotplug.d/usb/13-scmux
endef

$(eval $(call BuildPackage,simman2))

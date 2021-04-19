
include $(TOPDIR)/rules.mk

PKG_NAME:=simman
PKG_VERSION:=3
PKG_RELEASE:=2
PKG_BUILD_DIR:=$(BUILD_DIR)/simman
#CMAKE_INSTALL:=1

PKG_MAINTAINER:=Konstantin Sklyarec <atskyua@gmail.com> Vladimir Ovseychuk <vgovseychuk@gmail.com>

include $(INCLUDE_DIR)/package.mk
#include $(INCLUDE_DIR)/cmake.mk

define Package/simman
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Manager for SIM card in 3g modem 
  DEPENDS:=+libuci 
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib

define Package/simman/description
 This package contains utility for sim check manage.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/simman/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/simman $(1)/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/simman_getinfo $(1)/sbin/
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/simman.config $(1)/etc/config/simman
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/simman.init $(1)/etc/init.d/simman
	$(INSTALL_DIR) $(1)/etc/simman
	$(INSTALL_BIN) ./files/nbinfo.sh $(1)/etc/simman/nbinfo.sh
	$(INSTALL_BIN) ./files/setsim.sh $(1)/etc/simman/setsim.sh
	$(INSTALL_BIN) ./files/getccid.sh $(1)/etc/simman/getccid.sh
	$(INSTALL_BIN) ./files/getimei.sh $(1)/etc/simman/getimei.sh
	$(INSTALL_BIN) ./files/setfun.sh $(1)/etc/simman/setfun.sh
	$(INSTALL_BIN) ./files/getreg.sh $(1)/etc/simman/getreg.sh
	$(INSTALL_BIN) ./files/getgprsreg.sh $(1)/etc/simman/getgprsreg.sh
	$(INSTALL_BIN) ./files/getsiglev.sh $(1)/etc/simman/getsiglev.sh
	$(INSTALL_BIN) ./files/getpinstat.sh $(1)/etc/simman/getpinstat.sh
	$(INSTALL_BIN) ./files/getbasestid.sh $(1)/etc/simman/getbasestid.sh
	$(INSTALL_BIN) ./files/getnettype.sh $(1)/etc/simman/getnettype.sh
	$(INSTALL_BIN) ./files/getband.sh $(1)/etc/simman/getband.sh
	$(INSTALL_BIN) ./files/getsimcheck.sh $(1)/etc/simman/getsimcheck.sh
	$(INSTALL_BIN) ./files/getpackinfo.sh $(1)/etc/simman/getpackinfo.sh
	$(INSTALL_BIN) ./files/getcsq.sh $(1)/etc/simman/getcsq.sh
	$(INSTALL_BIN) ./files/check_prio.sh $(1)/etc/simman/check_prio.sh
	$(CP) ./files/*.gcom $(1)/etc/simman
	$(INSTALL_DIR) $(1)/etc/hotplug.d/usb
	$(INSTALL_DATA) ./files/simman.hotplug $(1)/etc/hotplug.d/usb/12-modem
	$(INSTALL_DATA) ./files/scmux.hotplug $(1)/etc/hotplug.d/usb/13-scmux
endef

$(eval $(call BuildPackage,simman))

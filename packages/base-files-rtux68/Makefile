#
# Copyright (C) 2007-2021 OpenWrt.org
# Copyright (C) 2010 Vertical Communications

include $(TOPDIR)/rules.mk

PKG_NAME:=base-files-rtux68
PKG_VERSION:=1
PKG_RELEASE:=0
PKG_BUILD_DIR:=$(BUILD_DIR)/base-files-lt70

include $(INCLUDE_DIR)/package.mk

define Package/base-files-rtux68
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Base config for TELEOFIS RTUx68
  DEPENDS:=
endef

define Package/base-files-rtux68/description
	 This package contains a utility for managing the interface using ping requests.
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/base-files-rtux68/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,base-files-rtux68))
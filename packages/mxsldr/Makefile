
include $(TOPDIR)/rules.mk

PKG_NAME:=mxsldr
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/mxsldr

include $(INCLUDE_DIR)/package.mk

define Package/mxsldr
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:= The i.MX233/i.MX28 USB Loader
  DEPENDS:=+libusb-1.0
endef

define Package/mxsldr/description
 This tool allows user to load U-Boot via BootROM USB download.
endef

TARGET_CFLAGS += \
    -I$(STAGING_DIR)/usr/include/libusb-1.0

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall"
endef

define Package/mxsldr/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mxsldr $(1)/usr/bin
endef

$(eval $(call BuildPackage,mxsldr))

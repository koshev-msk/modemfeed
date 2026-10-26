
include $(TOPDIR)/rules.mk

PKG_NAME:=xradio
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0-only
PKG_LICENSE_FILES:=

PKG_SOURCE_URL:=https://github.com/fifteenhex/xradio.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2025-06-01
PKG_SOURCE_VERSION:=6e170c96bf50a1088d5dd85fdfd67dc7f83e4eb9
PKG_HASH:=skip

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

#
# XR819 firmware download
#

define Download/xr819
  FILE:=xr819.tar.bz2
  URL:=https://github.com/melsem/openwrt-lede_xradio-xr819_soc-audio/raw/master/firmware/
  HASH:=bc9bae512267e04b6d00ffe18fe68a4181c9a5af35f765dfbabdeedd28c8f4f7
endef
$(eval $(call Download,xr819))

#
# Kernel module
#

define KernelPackage/xradio
  SUBMENU:=Wireless Drivers
  TITLE:=Xradio XR819 wireless driver
  DEPENDS:=@TARGET_sunxi +kmod-mac80211 +xr819-firmware
  KCONFIG:= \
	CONFIG_WLAN_VENDOR_XRADIO=m \
	CONFIG_XRADIO_USE_EXTENSIONS=y
  FILES:=$(PKG_BUILD_DIR)/xradio_wlan.ko
  AUTOLOAD:=$(call AutoProbe,xradio_wlan)
endef

NOSTDINC_FLAGS = \
	-I$(PKG_BUILD_DIR) \
	-I$(STAGING_DIR)/usr/include/mac80211-backport/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211-backport \
	-I$(STAGING_DIR)/usr/include/mac80211/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211 \
	-include backport/autoconf.h \
	-include backport/backport.h

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		$(PKG_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		modules
endef

$(eval $(call KernelPackage,xradio))

#
# XR819 firmware package
#

define Package/xr819-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=Xradio XR819 firmware
  DEPENDS:=@TARGET_sunxi
endef

define Package/xr819-firmware/description
 Firmware files for the Xradio XR819 wireless chipset.
endef

define Package/xr819-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(TAR) -C $(1)/lib/firmware -xjf $(DL_DIR)/xr819.tar.bz2
endef

$(eval $(call BuildPackage,xr819-firmware))


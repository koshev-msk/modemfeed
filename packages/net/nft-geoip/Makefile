#
# nftables geoip expression, reads the xt_geoip database
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=nft-geoip
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0-only

include $(INCLUDE_DIR)/package.mk

define KernelPackage/nft-geoip
  SUBMENU:=Netfilter Extensions
  TITLE:=nftables geoip expression (xt_geoip database)
  DEPENDS:=+kmod-nft-core
  FILES:=$(PKG_BUILD_DIR)/nft_geoip.ko
  AUTOLOAD:=$(call AutoProbe,nft_geoip)
endef

define KernelPackage/nft-geoip/description
  Kernel module providing the "geoip" nftables expression.
  Country databases are read from /usr/share/xt_geoip/XX.iv4 and XX.iv6.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,nft-geoip))

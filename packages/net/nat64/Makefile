include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=nat64
PKG_SOURCE_URL:=https://github.com/primis/nat64.git
PKG_SOURCE_DATE:=2021-06-30
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=1ba5895ff5f227fe9efd2d3765c4cdde6ac36899

PKG_MAINTAINER:=Hans Dedecker <dedeckeh@gmail.com>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/nat64
  DEPENDS:=@IPV6
  TITLE:=Stateless NAT64 translation kernel module
  SECTION:=kernel
  SUBMENU:=Network Support
  FILES:=$(PKG_BUILD_DIR)/nat64.ko
  AUTOLOAD:=$(call AutoLoad,33,nat64)
endef

include $(INCLUDE_DIR)/kernel-defaults.mk

define Build/Configure
	$(call Build/Configure/Default)
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)/" \
		MODFLAGS="-DMODULE -mlong-calls" \
		EXTRA_CFLAGS="-DNAT64_VERSION=\\\"$(PKG_SOURCE_VERSION)\\\"" \
		modules
endef

$(eval $(call KernelPackage,nat64))

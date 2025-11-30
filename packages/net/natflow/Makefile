#
# Copyright (C) 2017-2019 Chen Minqiang <ptpt52@gmail.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=natflow
PKG_VERSION:=20251024
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://github.com/ptpt52/natflow.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=eae16f2d9c0ade218279290ea09a00a2434a61e6
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_MAINTAINER:=Chen Minqiang <ptpt52@gmail.com>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/natflow
  CATEGORY:=X
  SUBMENU:=Fast Forward Stacks
  TITLE:=natflow kernel driver
  KCONFIG:= \
	CONFIG_NETFILTER_INGRESS=y
  FILES:=$(PKG_BUILD_DIR)/natflow.ko
  AUTOLOAD:=$(call AutoLoad,96,natflow)
  DEPENDS:= +kmod-ipt-conntrack +kmod-ipt-nat +kmod-ipt-ipset
endef

define KernelPackage/natflow/description
  fast nat forward kmod
endef

include $(INCLUDE_DIR)/kernel-defaults.mk

define Build/Compile/natflow
	+$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		modules
endef

define Build/Compile
	$(call Build/Compile/natflow)
endef

define Package/natflow-boot
  CATEGORY:=X
  SUBMENU:=Fast Forward Stacks
  TITLE:=natflow boot init scripts
  PKGARCH:=all
  DEPENDS:= +kmod-natflow
endef

define Package/natflow-boot/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/natflow-boot.init $(1)/etc/init.d/natflow-boot
endef

$(eval $(call KernelPackage,natflow))
$(eval $(call BuildPackage,natflow-boot))

#
# Based on package from https://github.com/Ysurac/openmptcprouter-feeds/blob/master/ndpi-netfilter2
# Copyright (C) 2018-2022 Ycarus (Yannick Chabanois) <ycarus@zugaina.org> for OpenMPTCProuter
# (C) 2023 modified by Konstantine Shevlakov <shevlakov@132lan.ru> OpenWrt modemfeed project
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ndpi-netfilter
PKG_REV:=4c1a23b0539ec15296029b149c53f5946d8b7280
PKG_REV_SHORT=$(shell echo $(PKG_REV) | cut -c1-6)
PKG_VERSION:=2026.03.30-flow_info-4~$(PKG_REV_SHORT)
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/vel21ripn/nDPI.git
PKG_SOURCE_VERSION:=$(PKG_REV)

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=libpcap

include $(INCLUDE_DIR)/package.mk

define Package/iptables-mod-ndpi
  SUBMENU:=Firewall
  SECTION:=net
  CATEGORY:=Network
  TITLE:=ndpi successor of nDPI
  URL:=http://www.ntop.org/products/ndpi/
  DEPENDS:=+iptables +iptables-mod-conntrack-extra +kmod-ipt-ndpi +libpcap
  MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
endef

define Package/iptables-mod-ndpi/description
	nDPI is a ntop-maintained superset of the popular OpenDPI library
endef

MAKE_PATH:=ndpi-netfilter

MAKE_FLAGS += \
	KERNEL_DIR="$(LINUX_DIR)" \
	MODULES_DIR="$(TARGET_MODULES_DIR)" \
	ARCH="$(LINUX_KARCH)" \
	KERNEL_RELEASE="$(KERNEL_PATCHVER)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	CC="$(TARGET_CC)" \
	EXTRA_CFLAGS="-D__KERNEL__ -DMODULE -I$(PKG_BUILD_DIR)/src/include -I$(PKG_BUILD_DIR)/src/lib/third_party/include -Wno-error -Wno-unused-function -Wno-implicit-fallthrough"

INCLUDE_SRC += -I../../src/include/ -I../../src/lib/third_party/include/
CFLAGS += -g -O2 -fPIC -DPIC -DNDPI_LIB_COMPILATION

#MAKE_FLAGS += \
#    NDPI_PATH=$(PKG_BUILD_DIR)/ndpi-netfilter

define Build/Configure
	(cd $(PKG_BUILD_DIR); \
		./autogen.sh; \
		./configure --with-pic --host=$(GNU_TARGET_NAME) \
		CFLAGS="$(TARGET_CFLAGS) -fPIC -DPIC" \
		CPPFLAGS="-I$(STAGING_DIR)/usr/include" \
		LDFLAGS="-L$(STAGING_DIR)/usr/lib" \
		LIBS="-lpcap")
endef

define Build/Compile
	$(MAKE) $(MAKE_FLAGS) -C $(PKG_BUILD_DIR)/ndpi-netfilter
endef

define Package/iptables-mod-ndpi/install
	$(INSTALL_DIR) $(1)/usr/lib/iptables
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ndpi-netfilter/ipt/libxt_ndpi.so $(1)/usr/lib/iptables
endef

define KernelPackage/ipt-ndpi
  SUBMENU:=Netfilter Extensions
  TITLE:= nDPI net netfilter module
  DEPENDS:=+kmod-nf-conntrack +kmod-nf-conntrack-netlink +kmod-ipt-compat-xtables +libpcap
  FILES:= $(PKG_BUILD_DIR)/ndpi-netfilter/src/xt_ndpi.ko
  KCONFIG:=\
	CONFIG_NF_CONNTRACK=y \
	CONFIG_NF_CONNTRACK_LABELS=y \
	CONFIG_NETFILTER_XT_MATCH_CONNLABEL=y
  AUTOLOAD:=$(call AutoProbe,xt_ndpi)
endef

$(eval $(call BuildPackage,iptables-mod-ndpi))
$(eval $(call KernelPackage,ipt-ndpi))

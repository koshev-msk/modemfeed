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
PKG_REV:=9a6412b70daf21034e1ae93980585a2edd79a70d
PKG_VERSION:=10242023-flow_info-4
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/vel21ripn/nDPI.git
PKG_SOURCE_VERSION:=$(PKG_REV)

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/iptables-mod-ndpi
  SUBMENU:=Firewall
  SECTION:=net
  CATEGORY:=Network
  TITLE:=ndpi successor of OpenDPI
  URL:=http://www.ntop.org/products/ndpi/
  DEPENDS:=+iptables +iptables-mod-conntrack-extra +kmod-ipt-ndpi +libpcap
  MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
endef

define Package/iptables-mod-ndpi/description
	nDPI is a ntop-maintained superset of the popular OpenDPI library
endef

CONFIGURE_CMD=./autogen.sh
CONFIGURE_ARGS += --with-pic

MAKE_PATH:=ndpi-netfilter

MAKE_FLAGS += \
	KERNEL_DIR="$(LINUX_DIR)" \
	MODULES_DIR="$(TARGET_MODULES_DIR)" \
	ARCH="$(LINUX_KARCH)" \
	KERNEL_RELEASE="$(KERNEL_PATCHVER)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	CC="$(TARGET_CC)" \

INCLUDE_SRC += -I../../src/include/ -I../../src/lib/third_party/include/
CFLAGS += -g -O2 -fPIC -DPIC -DNDPI_LIB_COMPILATION

NDPI_NETLIST += ndpi_network_list_compile
YAML_LIST = ndpi_network_list_*.yaml

#MAKE_FLAGS += \
#    NDPI_PATH=$(PKG_BUILD_DIR)/ndpi-netfilter

define Build/Compile
	(cd $(PKG_BUILD_DIR)/src/lib && $(HOSTCC) $(CFLAGS) $(INCLUDE_SRC) ndpi_network_list_compile.c -o $(NDPI_NETLIST) && \
		./$(NDPI_NETLIST) -o ndpi_network_list.c.inc $(YAML_LIST) && \
		$(MAKE) $(MAKE_FLAGS) -C $(PKG_BUILD_DIR)/ndpi-netfilter)
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
	CONFIG_LIVEPATCH=y \
	CONFIG_NF_CONNTRACK=y \
	CONFIG_NF_CONNTRACK_LABELS=y \
	CONFIG_NETFILTER_XT_MATCH_CONNLABEL=y
  AUTOLOAD:=$(call AutoProbe,xt_ndpi)
endef

$(eval $(call BuildPackage,iptables-mod-ndpi))
$(eval $(call KernelPackage,ipt-ndpi))

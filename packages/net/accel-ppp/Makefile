#
# Copyright (C) 2013-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=accel-ppp
PKG_RELEASE:=3
PKG_VERSION:=1.14.0

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/accel-ppp/accel-ppp.git
PKG_SOURCE_VERSION:=048d31cb446879e0d1a1471b4ab99135a92bf289

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=GPL-2.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk
include $(INCLUDE_DIR)/kernel.mk

# Strip -mips16 flags: libatomic does not provide __sync_*_8 in MIPS16 mode
TARGET_CFLAGS:=$(filter-out -mips16 -minterlink-mips16,$(TARGET_CFLAGS))
TARGET_LDFLAGS:=$(filter-out -mips16 -minterlink-mips16,$(TARGET_LDFLAGS))

# Base depends
DEPENDS_COMMON:=+libpcre2 +libopenssl +libpthread +librt +libatomic +libucontext

ifeq ($(CONFIG_ACCEL_PPP_PGSQL_LOG),y)
  DEPENDS_COMMON+= +libpq
endif

ifeq ($(CONFIG_ACCEL_PPP_SNMP),y)
  DEPENDS_COMMON+= +netsnmp
endif

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=VPN
  TITLE:=accel-ppp VPN server
  DEPENDS:=$(DEPENDS_COMMON)
  MENU:=1
endef

define Package/$(PKG_NAME)/config
  source "$(SOURCE)/Config.in"
endef

# CMake Config flags
CMAKE_OPTIONS += \
	-DKDIR=$(LINUX_DIR) \
	-DKARCH=$(LINUX_KARCH) \
	-DCROSS_COMPILE=$(TARGET_CROSS) \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_BUILD_TYPE=Release \
	-DLOG_PGSQL=$(if $(CONFIG_ACCEL_PPP_PGSQL_LOG),TRUE,FALSE) \
	-DLOG_FILE=$(if $(CONFIG_ACCEL_PPP_FILE_LOG),TRUE,FALSE) \
	-DSHAPER=$(if $(CONFIG_ACCEL_PPP_SHAPER),TRUE,FALSE) \
	-DRADIUS=$(if $(CONFIG_ACCEL_PPP_RADIUS),TRUE,FALSE) \
	-DNETSNMP=$(if $(CONFIG_ACCEL_PPP_SNMP),TRUE,FALSE) \
	-DLIB_SUFFIX= \
	-DBUILD_IPOE_DRIVER=$(if $(CONFIG_PACKAGE_kmod-$(PKG_NAME)-ipoe),TRUE,FALSE) \
	-DBUILD_VLAN_MON_DRIVER=$(if $(CONFIG_PACKAGE_kmod-$(PKG_NAME)-vlan-mon),TRUE,FALSE)

CMAKE_BINARY_DIR := $(PKG_BUILD_DIR)/build
TARGET_CFLAGS += -I$(STAGING_DIR)/opt/include -D_GNU_SOURCE -g -O2 -Wno-incompatible-pointer-types -Wno-deprecated-declarations
TARGET_LDFLAGS += -lpthread -latomic -lucontext -Wl,-rpath,/usr/lib/accel-ppp

define KernelPackage/$(PKG_NAME)-ipoe
  SUBMENU:=Accel-PPP kernel modules
  TITLE:=Accel-PPP IPoE kernel module
  DEPENDS:=+kmod-ppp +kmod-pppox
  FILES:=$(CMAKE_BINARY_DIR)/drivers/ipoe/driver/ipoe.ko
  AUTOLOAD:=$(call AutoProbe,ipoe)
endef

define KernelPackage/$(PKG_NAME)-vlan-mon
  SUBMENU:=Accel-PPP kernel modules
  TITLE:=Accel-PPP vlan-mon kernel module
  DEPENDS:=+kmod-8021q
  FILES:=$(CMAKE_BINARY_DIR)/drivers/vlan_mon/driver/vlan_mon.ko
  AUTOLOAD:=$(call AutoProbe,vlan_mon)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) \
		$(1)/usr/sbin \
		$(1)/usr/bin \
		$(1)/usr/lib \
		$(1)/usr/lib/accel-ppp \
		$(1)/usr/share/accel-ppp \
		$(1)/usr/share/accel-ppp/radius \
		$(1)/usr/share/accel-ppp/l2tp \
		$(1)/etc/init.d \
		$(1)/etc/accel-ppp

	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/accel-pppd \
		$(1)/usr/sbin/

	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/accel-cmd \
		$(1)/usr/bin/

	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/lib/accel-ppp/*.so \
		$(1)/usr/lib/accel-ppp/

	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/accel-ppp/l2tp/dictionary.* \
		$(1)/usr/share/accel-ppp/l2tp/ || true

	$(INSTALL_BIN) \
		files/accel-ppp.init \
		$(1)/etc/init.d/accel-ppp

	$(INSTALL_DATA) \
		files/accel-ppp.conf \
		$(1)/etc/accel-ppp/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call KernelPackage,$(PKG_NAME)-ipoe))
$(eval $(call KernelPackage,$(PKG_NAME)-vlan-mon))

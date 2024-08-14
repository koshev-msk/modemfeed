#B
#
# Copyright (C) 2013-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=accel-ppp
PKG_RELEASE:=2
PKG_VERSION:=1.13.0

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/accel-ppp/accel-ppp.git
PKG_SOURCE_VERSION:=dbffa39b4304f1be5e099f056a9e5001c3100393

PKG_MAINTAINER:=Alex Paraskeva <sim201010@gmail.com>
PKG_LICENSE:=GPL-2.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

#PKG_INSTALL:=1
#PKG_CONFIG_DEPENDS:=CONFIG_PACKAGE_accel-ppp_$(BUILD_VARIANT)_ext_cer_id

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  TITLE:=accel-ppp VPN server
  DEPENDS:=+libpcre +libopenssl +libpthread +librt +libatomic +libucontext
endef

CMAKE_OPTIONS += \
	-DBUILD_DRIVER=FALSE \
	-DKDIR=$(LINUX_DIR) \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_BUILD_TYPE=Release \
	-DLOG_PGSQL=FALSE \
	-DSHAPER=TRUE \
	-DRADIUS=TRUE \
	-DNETSNMP=FALSE \
	-DLOG_FILE=TRUE \
	-DLIB_SUFFIX= \

CMAKE_BINARY_DIR := $(PKG_BUILD_DIR)/build
TARGET_CFLAGS += -I$(STAGING_DIR)/opt/include -D_GNU_SOURCE -g -O2 -Wno-incompatible-pointer-types
TARGET_LDFLAGS += -lpthread -latomic -lucontext


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) \
		$(1)/usr/sbin \
		$(1)/usr/bin \
		$(1)/usr/share/accel-ppp \
		$(1)/usr/share/accel-ppp/radius \
		$(1)/usr/share/accel-ppp/l2tp \
		$(1)/usr/lib/accel-ppp \
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
		$(PKG_INSTALL_DIR)/usr/share/accel-ppp/radius/dictionary.* \
		$(1)/usr/share/accel-ppp/radius/

	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/accel-ppp/l2tp/dictionary.* \
		$(1)/usr/share/accel-ppp/l2tp/

	$(INSTALL_BIN) \
		files/accel-ppp.init \
		$(1)/etc/init.d/accel-ppp

	$(INSTALL_DATA) \
		files/accel-ppp.conf \
		$(1)/etc/accel-ppp

endef

define Package/$(PKG_NAME)/postinst
if [ -f /usr/lib/accel-ppp/libtriton.so ]; then
	ln -s /usr/lib/accel-ppp/libtriton.so /usr/lib/
fi
endef

define Package/$(PKG_NAME)/prerm
rm -rf /usr/lib/libtriton.so
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

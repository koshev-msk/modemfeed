#
# Copyright (C) 2017 - 2018 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=asterisk-chan-quectel
PKG_VERSION:=2.1-20211201
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://github.com/IchthysMaranatha/asterisk-chan-quectel.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=2f94f3582e49f3969d272acfb351cd16a24d3ec9
PKG_SOURCE_PROTO:=git

PKG_FIXUP:=autoreconf

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYRIGHT.txt LICENSE.txt
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

define Build/Prepare
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
$(CP) ./src/* $(PKG_BUILD_DIR)/
endef



MODULES_DIR:=/usr/lib/asterisk/modules

include $(INCLUDE_DIR)/package.mk
# asterisk-chan-quectel needs iconv
include $(INCLUDE_DIR)/nls.mk

define Package/asterisk-chan-quectel/Default
  SUBMENU:=Telephony
  SECTION:=net
  CATEGORY:=Network
  URL:=https://github.com/IchthysMaranatha/asterisk-chan-quectel
  DEPENDS:=asterisk $(ICONV_DEPENDS) +kmod-usb-acm +kmod-usb-serial +kmod-usb-serial-option +libusb-1.0 +alsa-lib
  TITLE:=Asterisk Mobile Telephony Module
endef

define Package/asterisk16-chan-quectel
$(call Package/asterisk-chan-quectel/Default)
  DEPENDS+=asterisk16
  VARIANT:=asterisk16
endef

define Package/description/Default
 Asterisk channel driver for mobile telephony.
endef

Package/asterisk16-chan-quectel/description = $(Package/description/Default)

ifeq ($(BUILD_VARIANT),asterisk16)
  CHAN_quectel_AST_HEADERS:=$(STAGING_DIR)/usr/include/asterisk-16/include
  CONFIGURE_ARGS+= \
	  --with-astversion=16
endif


CONFIGURE_ARGS+= \
	--with-asterisk=$(CHAN_quectel_AST_HEADERS)

TARGET_CFLAGS+= \
	-I$(CHAN_quectel_AST_HEADERS)

MAKE_FLAGS+=LD="$(TARGET_CC)"

CONFIGURE_VARS += \
	DESTDIR="$(MODULES_DIR)" \
	ac_cv_type_size_t=yes \
	ac_cv_type_ssize_t=yes


define Package/conffiles/Default
/etc/asterisk/quectel.conf
endef

Package/asterisk16-chan-quectel/conffiles = $(Package/conffiles/Default)

define Package/Install/Default
	$(INSTALL_DIR) $(1)/etc/asterisk
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/etc/quectel.conf $(1)/etc/asterisk
	$(INSTALL_DIR) $(1)$(MODULES_DIR)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/chan_quectel.so $(1)$(MODULES_DIR)
endef

Package/asterisk16-chan-quectel/install = $(Package/Install/Default)

$(eval $(call BuildPackage,asterisk16-chan-quectel))

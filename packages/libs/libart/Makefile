#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# Used by rrdtool only, no need to update it

include $(TOPDIR)/rules.mk

PKG_NAME:=libart
PKG_VERSION:=2.3.21
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)_lgpl-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=@GNOME/$(PKG_NAME)_lgpl/2.3
PKG_HASH:=fdc11e74c10fc9ffe4188537e2b370c0abacca7d89021d4d303afdf7fd7476fa

PKG_LICENSE:=LGPL-2.0-or-later
PKG_LICENSE_FILES:=COPYING

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)_lgpl-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libart
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=An high-performance 2D graphics library
  URL:=http://www.levien.com/libart/
endef

define Package/libart/description
 A library of functions for 2D graphics supporting a superset of the
 PostScript imaging model, designed to be integrated with graphics,
 artwork, and illustration programs. It is written in optimized C, and
 is fully compatible with C++. With a small footprint of 10,000 lines of
 code, it is especially suitable for embedded applications.
endef

CONFIGURE_ARGS += \
	--enable-shared \
	--disable-static

TARGET_CFLAGS += $(FPIC)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/bin $(2)/bin $(1)/usr/include $(1)/usr/lib/pkgconfig
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/libart2-config \
		$(1)/usr/bin/
	$(SED) \
		's,^\(prefix\|exec_prefix\)=.*,\1=$(STAGING_DIR)/usr,g' \
		$(1)/usr/bin/libart2-config
	$(LN) $(STAGING_DIR)/usr/bin/libart2-config $(2)/bin/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/libart-2.0 \
			$(1)/usr/include/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libart_lgpl_2.so* \
		$(1)/usr/lib/

	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libart-2.0.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libart/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libart_lgpl_2.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libart))

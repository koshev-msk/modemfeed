#
# Copyright (C) 2016 Nikil Mehta <nikil.mehta@gmail.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=qtools
PKG_VERSION:=0.0.1
PKG_RELEASE:=0~cross

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=LICENSE
PKG_LICENSE_FILES:=COPYING

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/intelfx/qtools.git
PKG_SOURCE_VERSION:=c35166595dd252f23fd6de9b58a94612a1d12627

PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)


PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=telephony
  CATEGORY:=Utils
  TITLE:=Tools modems based on the Qualcom chipset
  URL:=https://github.com/forth32/qtools
  DEPENDS:=+libreadline +libncurses
endef

define Package/$(PKG_NAME)/description
	A set of tools for working with flash modems based on the Qualcom chipset The set consists of a package of utilities and a set of patched loaders.
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/share
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mibibsplit \
			$(PKG_BUILD_DIR)/qbadblock \
			$(PKG_BUILD_DIR)/qblinfo \
			$(PKG_BUILD_DIR)/qcommand \
			$(PKG_BUILD_DIR)/qdload \
			$(PKG_BUILD_DIR)/qefs \
			$(PKG_BUILD_DIR)/qflashparm \
			$(PKG_BUILD_DIR)/qident \
			$(PKG_BUILD_DIR)/qnvram \
			$(PKG_BUILD_DIR)/qrflash \
			$(PKG_BUILD_DIR)/qrmem \
			$(PKG_BUILD_DIR)/qwdirect \
			$(PKG_BUILD_DIR)/qwflash $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/*.sh $(1)/usr/share
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

include $(TOPDIR)/rules.mk

PKG_NAME:=maxminddb-dump-country
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>
PKG_LICENSE:=LICENSE
PKG_LICENSE_FILES:=COPYING

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/vel21ripn/maxminddb-dump-country.git
PKG_SOURCE_VERSION:=0f4c8aa54e30c65ee73b2d08c2a44a2f0ad1acd0

PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)

#PKG_INSTALL:=1
#PKG_BUILD_PARALLEL:=1

#PKG_FIXUP:=autoreconf
#PKG_BUILD_DEPENDS:=libmaxminddb/host

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:= +libmaxminddb
  TITLE:=Tool for convert maxmindDB to xt_geoip
  URL:=https://github.com/vel21ripn/maxminddb-dump-country
endef

define Package/$(PKG_NAME)/description
	Dump maxmind DB for linux iptables xt_geoip
	The program is designed to convert the GeoLite2-Country.mmdb file into a dataset for xtables-addons/xt_geoip.
	It is also possible to get files GeoLite2-Country-Blocks-IPv4.csv, GeoLite2-Country-Blocks-IPv6.csv, GeoLite2-Country-Locations-en.csv
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -lmaxminddb \
		-o $(PKG_BUILD_DIR)/xt_geoip_build_maxmind $(PKG_BUILD_DIR)/xt_geoip_build_maxmind.c 
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/xt_geoip_build_maxmind $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

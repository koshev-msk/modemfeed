include $(TOPDIR)/rules.mk

PKG_NAME:=rrdtool1n
PKG_VERSION:=1.2.30
PKG_RELEASE:=1

PKG_SOURCE:=rrdtool-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://oss.oetiker.ch/rrdtool/pub/archive
PKG_HASH:=3190efea410a6dd035799717948b2df09910f608d72d23ee81adad4cd0184ae9
PKG_BUILD_DIR:=$(BUILD_DIR)/rrdtool-$(PKG_VERSION)

PKG_LICENSE:=GPL-2.0
PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/rrdtool1n
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Round Robin Database Tool
  URL:=https://oss.oetiker.ch/rrdtool/
  DEPENDS:=+librrd1n
  CONFLICTS:=rrdtool1
endef

define Package/librrd1n
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=RRDtool library
  DEPENDS:=+libpng +zlib +libfreetype +libart
  CONFLICTS:=librrd1
endef

define Package/rrdtool1n-fonts
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+rrdtool1n
  TITLE:=RRDtool default font (DejaVu Sans Mono)
endef

define Package/rrdtool1n/description
 RRDtool. For graphing install rrdtool1n-fonts too, otherwise
 rrdtool graph fails to load its default font.
endef

CONFIGURE_ARGS += \
	--disable-rrdcgi \
	--disable-tcl \
	--disable-perl \
	--disable-python \
	--disable-ruby \
	--disable-lua \
	--disable-static \
	--without-x

CONFIGURE_VARS += \
	rd_cv_ieee_works=yes \
	rd_cv_null_realloc=nonnull

TARGET_CFLAGS += $(FPIC)

MAKE_FLAGS += mkdir_p="mkdir -p"

define Build/Configure
	$(call Build/Configure/Default)
endef

define Package/rrdtool1n/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/rrdtool $(1)/usr/bin/
endef

define Package/librrd1n/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/librrd*.so* $(1)/usr/lib/
endef

define Package/rrdtool1n-fonts/install
	$(INSTALL_DIR) $(1)/usr/share/rrdtool/fonts
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/src/DejaVuSansMono-Roman.ttf \
		$(1)/usr/share/rrdtool/fonts/
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/include/rrd.h $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/librrd*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,rrdtool1n))
$(eval $(call BuildPackage,librrd1n))
$(eval $(call BuildPackage,rrdtool1n-fonts))

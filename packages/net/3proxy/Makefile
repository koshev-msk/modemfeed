include $(TOPDIR)/rules.mk

PKG_NAME:=3proxy
PKG_VERSION:=0.9.8
PKG_RELEASE:=1

PKG_MAINTAINER:=muziling <lls924@gmail.com>
PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=LICENSE

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/z3APA3A/3proxy.git
PKG_SOURCE_VERSION:=122ca26249aaaac9156e0805891555c70e19f2b3

PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/config
  source "$(SOURCE)/Config.in"
endef


ifeq ($(CONFIG_PACKAGE_3proxy_WOLFSSL),y)
  SSL_DEPENDS:=+libwolfssl
  OPENSSL_CHECK_VAL:=false
  WOLFSSL_CHECK_VAL:=true
else
  SSL_DEPENDS:=+libopenssl
  OPENSSL_CHECK_VAL:=true
  WOLFSSL_CHECK_VAL:=false
endif

define Package/$(PKG_NAME)
  SUBMENU:=Web Servers/Proxies
  SECTION:=net
  CATEGORY:=Network
  TITLE:=3proxy OpenWRT package
  DEPENDS:=$(SSL_DEPENDS) +libpthread +libpcre2
  MENU:=1
endef

define Package/lib$(PKG_NAME)
  SUBMENU:=Web Servers/Proxies
  DEPENDS:=+3proxy
  TITLE:=3proxy libraries
endef

define Package/$(PKG_NAME)-common
  SUBMENU:=Web Servers/Proxies
  DEPENDS:=+3proxy
  TITLE:=3proxy addional servers
endef

define Package/$(PKG_NAME)-sql
  PKGARCH:=all
  DEPENDS:=+3proxy
  TITLE:=3proxy sql template
endef

define Package/$(PKG_NAME)/description
	3APA3A 3proxy tiny proxy servers
endef

define Build/Compile
    $(MAKE) -C $(PKG_BUILD_DIR) -f Makefile.Linux \
        CC="$(TARGET_CC)" \
        EXTRA_CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -fPIC -fno-strict-aliasing -c -pthread \
		-DNOODBC \
		-DGETHOSTBYNAME_R \
		-D_THREAD_SAFE \
		-D_REENTRANT \
		-DFD_SETSIZE=4096 \
		-DWITH_POLL \
		-DWITH_NETFILTER \
		-DWITHSPLICE \
		-D_GNU_SOURCE" \
        LDFLAGS="$(TARGET_LDFLAGS)" \
        PLUGINS="StringsPlugin TrafficPlugin TransparentPlugin" \
        OPENSSL_CHECK=$(OPENSSL_CHECK_VAL) \
        WOLFSSL_CHECK=$(WOLFSSL_CHECK_VAL) \
        PCRE_CHECK=true \
        PAM_CHECK=false
endef

define Package/$(PKG_NAME)/conffiles
/etc/config/3proxy
/etc/3proxy.cfg
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/3proxy \
			$(1)/usr/bin/
	$(CP) ./files/* $(1)/
endef

define Package/lib$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/lib/3proxy
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/*.so $(1)/usr/lib/3proxy
endef

define Package/$(PKG_NAME)-common/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/$(PKG_NAME)_* \
		$(1)/usr/bin
endef

define Package/$(PKG_NAME)-sql/install
	$(INSTALL_DIR) $(1)/usr/share/3proxy
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cfg/sql/* $(1)/usr/share/3proxy
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,lib$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)-common))
$(eval $(call BuildPackage,$(PKG_NAME)-sql))


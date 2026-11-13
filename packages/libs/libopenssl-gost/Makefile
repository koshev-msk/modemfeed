
include $(TOPDIR)/rules.mk

PKG_NAME:=libopenssl-gost
PKG_VERSION:=3.0.5
PKG_RELEASE:=3

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/gost-engine/engine.git
PKG_SOURCE_VERSION:=09fdd535fae018815651f34af6462fc280c3002e

PKG_MAINTAINER:=
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=openssl
PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += \
	-DCMAKE_BUILD_TYPE=Release \
	-DOPENSSL_ROOT_DIR=$(STAGING_DIR)/usr \
	-DOPENSSL_ENGINES_DIR=/usr/lib/engines-3 \
	-DGOST_BUILD_ENGINE=ON \
	-DGOST_BUILD_STATIC_ENGINE=OFF \
	-DGOST_BUILD_PROVIDER=ON \
	-DGOST_BUILD_TESTS=OFF \
	-DADDCARRY_U64_EXITCODE:STRING=0 \
	-DRELAXED_ALIGNMENT_EXITCODE:STRING=0

define Package/libopenssl-gost-engine
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=OpenSSL GOST Engine
  URL:=https://github.com/gost-engine/engine
  DEPENDS:=+libopenssl
endef

define Package/libopenssl-gost-engine/description
 GOST cryptographic algorithms implementation
 for OpenSSL 3.x ENGINE interface.
endef

define Package/libopenssl-gost-provider
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=OpenSSL GOST Provider
  URL:=https://github.com/gost-engine/engine
  DEPENDS:=+libopenssl
endef

define Package/libopenssl-gost-provider/description
  GOST cryptographic algorithms implementation
  for OpenSSL 3.x provider interface.
endef

define Package/gost-utils
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libopenssl-gost-engine
  TITLE+= GOST utilities
endef



define Package/libopenssl-gost-engine/install
	$(INSTALL_DIR) $(1)/usr/lib/engines-3 \
		$(1)/etc/ssl/modules.cnf.d \
		$(1)/etc/uci-defaults
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/engines-3/gost.so \
		$(1)/usr/lib/engines-3/
	$(INSTALL_DATA) ./files/gost.cnf $(1)/etc/ssl/modules.cnf.d/
	$(INSTALL_BIN) ./files/gost-engine.defaults \
		$(1)/etc/uci-defaults/90-libopenssl-gost-engine
endef

define Package/libopenssl-gost-engine/postinst
#!/bin/sh
[ -s $${IPKG_INSTROOT}/lib/functions.sh ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh
default_postinst $$0 $$@
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/openssl restart >/dev/null 2>&1
exit 0
endef

define Package/libopenssl-gost-engine/prerm
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] || {
	uci -q delete openssl.gost
	uci commit openssl
	/etc/init.d/openssl restart >/dev/null 2>&1
}
exit 0
endef

define Package/libopenssl-gost-provider/install
	$(INSTALL_DIR) $(1)/usr/lib/ossl-modules \
		$(1)/etc/ssl/modules.cnf.d \
		$(1)/etc/uci-defaults
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/ossl-modules/gostprov.so \
		$(1)/usr/lib/ossl-modules/
	$(INSTALL_DATA) ./files/gostprov.cnf $(1)/etc/ssl/modules.cnf.d/
	$(INSTALL_BIN) ./files/gost-provider.defaults \
		$(1)/etc/uci-defaults/90-libopenssl-gost-provider
endef

define Package/gost-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/gost12sum \
		$(PKG_INSTALL_DIR)/usr/bin/gostsum \
		$(1)/usr/bin/
endef

define Package/libopenssl-gost-provider/postinst
#!/bin/sh
[ -s $${IPKG_INSTROOT}/lib/functions.sh ] || exit 0
. $${IPKG_INSTROOT}/lib/functions.sh
default_postinst $$0 $$@
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/openssl restart >/dev/null 2>&1
exit 0
endef

define Package/libopenssl-gost-provider/prerm
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] || {
	uci -q delete openssl.gostprov
	uci commit openssl
	/etc/init.d/openssl restart >/dev/null 2>&1
}
exit 0
endef

$(eval $(call BuildPackage,libopenssl-gost-engine))
$(eval $(call BuildPackage,libopenssl-gost-provider))
$(eval $(call BuildPackage,gost-utils))

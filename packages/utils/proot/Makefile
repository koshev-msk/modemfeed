# package/proot/Makefile для OpenWrt
include $(TOPDIR)/rules.mk

PKG_NAME:=proot
PKG_VERSION:=5.4.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/proot-me/proot.git
PKG_SOURCE_VERSION:=v$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-version.mk

define Package/proot
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=PRoot - chroot, mount --bind, and binfmt_misc
  DEPENDS:=+libc +libgcc +libstdcpp +libtalloc +libarchive +libpython3
endef

define Package/proot/description
  PRoot is a user-space implementation of chroot, mount --bind, and binfmt_misc.
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		CC=$(TARGET_CC) \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS) -ltalloc -lpython$(PYTHON3_VERSION)" \
		WITH_PYTHON=yes \
		proot
endef

define Package/proot/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/proot $(1)/usr/bin/
endef

$(eval $(call BuildPackage,proot))

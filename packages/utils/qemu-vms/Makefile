include $(TOPDIR)/rules.mk

PKG_NAME:=qemu-vms
PKG_VERSION:=0.0.1
PKG_RELEASE:=1
PKG_MAINTAINER:=Konstantine Shevlakov <shevlakov@132lan.ru>

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  PKGARCH:=all
  TITLE:=QEMU Virtual Machines Simple (VMS)
  DEPENDS:= +qemu-x86_64-softmmu \
		+qemu-bridge-helper \
		+minicom \
		+socat
endef

define Package/$(PKG_NAME)/description
    Scripts to run Virtual Machines using QEMU via uci shell.
  Support mupltiple run stop and manage machnes via tty.
endef

define Package/$(PKG_NAME)/conffiles
	/etc/config/qemu-vms
endef


define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef


define Package/$(PKG_NAME)/install
	$(CP) ./root/* $(1)/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))


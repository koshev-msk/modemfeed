include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=xt_tls
PKG_VERSION:=0.3.5
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/Lochnair/xt_tls.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=692e50651ea024dc6ceddc764ae3391e93a99b67

PKG_MAINTAINER:=Nils Andreas Svee <me@lochnair.net>
PKG_LICENSE=GPL-3.0
PKG_LICENSE_FILES=LICENSE

include $(INCLUDE_DIR)/package.mk

# Package definition; instructs on how and where our package will appear in the overall configuration menu ('make menuconfig')
define Package/iptables-mod-tls
	SECTION:=net
	CATEGORY:=Network
	TITLE:=iptables library for filtering TLS connections
	DEPENDS:=libxtables
endef

# Package description; a more verbose description on what our package does
define Package/iptables-mod-tls/description
	iptables module for filtering TLS connections
endef

define KernelPackage/ipt-tls
	SUBMENU:=Other modules
	TITLE:=iptables module for filtering TLS connections
	FILES:=$(PKG_BUILD_DIR)/src/xt_tls.ko
	DEPENDS:=kmod-nf-ipt
endef

define KernelPackage/ipt-tls/description
	A iptables module for filtering TLS connections.
endef

MAKE_FLAGS += \
	      KDIR=$(LINUX_DIR) \
	      ARCH=$(LINUX_KARCH) \
	      M="$(PKG_BUILD_DIR)/src"

# Package install instructions; create a directory inside the package to hold our executable, and then copy the executable we built previously into the folder
define Package/iptables-mod-tls/install
	$(INSTALL_DIR) $(1)/usr/lib/iptables
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipt/libxt_tls.so $(1)/usr/lib/iptables
endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,iptables-mod-tls))
$(eval $(call KernelPackage,ipt-tls))

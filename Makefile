include $(TOPDIR)/rules.mk

LUCI_TITLE:=3proxy simple webUI
LUCI_DEPENDS:=+3proxy
PKG_LICENSE:=GPLv3
PKG_VERSION:=0.0.1
PKG_RELEASE:=3

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

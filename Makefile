include $(TOPDIR)/rules.mk

LUCI_TITLE:=TelegramBot simple webUI
LUCI_DEPENDS:=+telegrambot
PKG_LICENSE:=GPLv3
PKG_VERSION:=0.0.1
PKG_RELEASE:=3

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

include $(TOPDIR)/rules.mk

LUCI_TITLE=Information dashboard for 3G/LTE dongle
LUCI_DEPENDS=+comgt
PKG_LICENSE=GPLv3

include ../../luci.mk

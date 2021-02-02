include $(TOPDIR)/rules.mk

LUCI_TITLE:=Web UI for smstools3
LUCI_DEPENDS:=+smstools3 +iconv
PKG_LICENSE:=GPLv3

define Package/luci-app-smstools3/postrm
	rm -f /tmp/luci-indexcache
endef

include $(TOPDIR)/feeds/luci/luci.mk

define Package/luci-app-smstools3/conffiles
	/etc/config/smstools3
endef

define Package/luci-app-smstools3/postinst
	if [ -f /etc/init.d/smstools3 ]; then
		mv /etc/init.d/smstools3 /usr/share/luci-app-smstools3/smstools3.init.orig
		cp /usr/share/luci-app-smstools3/smstools3 /etc/init.d/
	fi
endef

define Package/luci-app-smstools3/prerm
	mv /usr/share/luci-app-smstools3/smstools3.init.orig /etc/init.d/smstools3
endef

# call BuildPackage - OpenWrt buildroot signature

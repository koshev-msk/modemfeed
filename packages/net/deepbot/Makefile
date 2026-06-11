include $(TOPDIR)/rules.mk

PKG_NAME:= deepbot
PKG_VERSION:= 0.0.2
PKG_RELEASE:= 1

include $(INCLUDE_DIR)/package.mk

define Package/deepbot
  SECTION:= utils
  CATEGORY:= Utilities
  TITLE:= DeepSeek AI Telegram Bot
  DEPENDS:= +lua +luasocket +lua-cjson +lsqlite3 +curl
  PKGARCH:= all
endef

define Package/deepbot/description
  Telegram bot powered by Open AI models. DeepSeek default.
  Supports private and group chats, conversation history,
  daily token limits, SOCKS5/HTTP proxy, white lists.
  Lua implementation from
  https://github.com/Bimaoitsuki/deepseek-telegram-bot-
endef

define Build/Compile
endef

define Package/deepbot/conffiles
/etc/config/deepbot
endef

define Package/deepbot/install
	$(INSTALL_DIR)  $(1)/usr/bin \
		$(1)/etc/init.d \
		$(1)/etc/config \
		$(1)/usr/share/deepbot
	$(INSTALL_BIN)  ./files/usr/bin/deepbot.lua \
		$(1)/usr/bin/deepbot.lua
	$(INSTALL_BIN)  ./files/etc/init.d/deepbot \
		$(1)/etc/init.d/deepbot
	$(INSTALL_DATA) ./files/etc/config/deepbot \
		$(1)/etc/config/deepbot
endef

$(eval $(call BuildPackage,deepbot))

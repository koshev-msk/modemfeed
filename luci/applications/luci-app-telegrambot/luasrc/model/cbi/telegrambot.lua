-- Copyright 2008 Yanira <forum-2008@email.de>
-- Copyright 2012 Jo-Philipp Wich <jow@openwrt.org>
-- Licensed to the public under the Apache License 2.0.

require("luci.sys")
local apply = luci.http.formvalue ("cbi.apply")
local m, s

s = Map("telegrambot", translate("TelegramBot"), translate("Telegram bot for router with firmware Lede/Openwrt."))
m = s:section(TypedSection)
m.anonymous = true
m.addremove = false
enable = m:option(Flag, "enabled", translate("Enable"), translate ("Enable Bot"))
enable.value = 1
enable.value = 0
enable.default = 0
token = m:option(Value, "bot_token", translate("Bot Token"), translate("Token ID your Telegram Bot"))
token.password = true
chatid = m:option(Value, "chat_id", translate("Bot ID"), translate("Chat ID your Telegram Bot"))
timeout = m:option(Value, "timeout", translate("Time Out"), translate("Time Out respone Bot in s."))
ptime = m:option(Value, "polling_time", translate("Polling Time"), translate ("Polling Time in s."))
plugins = m:option(Value, "plugins", translate("Plugins"), translate("Path to plugins directory."))
plugins.default = '/usr/lib/telegrambot/plugins'
hlog = m:option(Value, "log_file", translate("Log File"), translate ("Path to Logfile"))
hlog.default = '/tmp/telegrambot.log'

if apply then
        local restart = "/etc/init.d/telegrambot restart"
        luci.sys.exec(restart)
end

return s

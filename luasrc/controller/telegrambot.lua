module("luci.controller.telegrambot", package.seeall)

function index()
        if not nixio.fs.access("/etc/config/telegrambot") then
                return
        end
        local page
        page = entry({"admin", "services", "telegrambot"}, cbi("telegrambot"), _("TelegramBot"), 82)
end

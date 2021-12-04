module("luci.controller.telegrambot", package.seeall)

function index()
        if not nixio.fs.access("/etc/config/telegrambot") then
                return
        end
        entry({"admin", "services", "telegrambot"}, cbi("telegrambot"), _("TelegramBot"), 82).acl_depends={"unauthenticated"}
end

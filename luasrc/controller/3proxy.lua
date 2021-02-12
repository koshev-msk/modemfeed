module("luci.controller.3proxy", package.seeall)

function index()
	entry({"admin", "services", "3proxy"}, alias("admin", "services", "3proxy", "config"),  _("3proxy"), 93)
	entry({"admin", "services", "3proxy", "config"}, cbi("3proxy/3proxy"), _("Setup"), 94)
	entry({"admin", "services", "3proxy", "template"}, form("3proxy/3proxy_tpl"), _("Edit Config File"), 95)
end

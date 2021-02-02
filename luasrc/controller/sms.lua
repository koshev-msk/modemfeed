local nixio = require "nixio"

module("luci.controller.sms", package.seeall)

local utl = require "luci.util"

function index()
	entry({"admin", "modem"},  firstchild(), "Modem", 45).dependent = false
	entry({"admin", "modem", "sms"}, alias ("admin", "modem", "sms", "in_sms"), translate("SMS"), 11)
	entry({"admin", "modem", "sms", "in_sms"}, template("modem/sms/in"), translate("Incoming"), 22)
	entry({"admin", "modem", "sms", "out_sms"}, template("modem/sms/out"), translate("Outcoming"),23)
	entry({"admin", "modem", "sms", "send_sms"}, template("modem/sms/send"), translate("Send"), 24)
	entry({"admin", "modem", "sms", "setup_sms"}, cbi("modem/sms"), translate("Setup"), 25)
	entry({"admin", "modem", "sms", "in_erase"}, template("modem/sms/in_erase"), nil).leaf = true
	entry({"admin", "modem", "sms", "out_erase"}, template("modem/sms/out_erase"), nil).leaf = true
	entry({"admin", "modem", "push_sms"}, call("action_send_sms"))
end


function action_send_sms()
	local set = luci.http.formvalue("set")
	number = (string.sub(set, 1, 20))
	txt = string.sub(set, 21)
	message = string.gsub(txt, "\n", " ")
	os.execute("/usr/bin/sendsms " ..number.." '"..message.."'")
end

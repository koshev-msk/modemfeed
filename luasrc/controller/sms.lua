local nixio = require "nixio"

module("luci.controller.sms", package.seeall)

local utl = require "luci.util"

function index()
	entry({"admin", "modem"},  firstchild(), "Modem", 45).dependent = false
	entry({"admin", "modem", "in_sms"}, template("modem/sms/in"), "SMS", 11).leaf = true
	entry({"admin", "modem", "out_sms"}, template("modem/sms/out"), nil).leaf = true
	entry({"admin", "modem", "send_sms"}, template("modem/sms/send"), nil).leaf = true
	entry({"admin", "modem", "in_erase"}, template("modem/sms/in_erase"), nil).leaf = true
	entry({"admin", "modem", "out_erase"}, template("modem/sms/out_erase"), nil).leaf = true
	entry({"admin", "modem", "push_sms"}, call("action_send_sms"))
end


function action_send_sms()
	local set = luci.http.formvalue("set")
	number = (string.sub(set, 1, 20))
	txt = string.sub(set, 21)
	message = string.gsub(txt, "\n", " ")
	os.execute("/usr/bin/sendsms " ..number.." '"..message.."'")
end
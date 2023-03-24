-- Copyright 2023 Konstantine Shevlyakov <shevlakov@132lan.ru>
-- Licensed to the GNU General Public License v3.0.

require("nixio.fs")

local m, d, s

local m = Map("smstools3", translate("Smstools3: command list interface."))

local f = m:section(TypedSection, "root_phone", "<p>&nbsp;</p>" .. translate("Root Phone numbers"))
f.anonymous = true
f.rmempty = true
data = f:option(DynamicList, "phone", translate("Phone number"),
	translate("List phone numbers without +"))


l = m:section(TypedSection, "root_phone", "<p>&nbsp;</p>" .. translate("Command List"))
l.anonymous = true

local s = m:section(TypedSection, "command")
desc = s:option(Value, "desc", translate("Description"))
sms = s:option(Value, "command", translate("SMS command"))
exec = s:option(Value, "exec", translate("Execute"))
delay_en = s:option(Flag, "delay_en", translate("Delay"))
delay = s:option(Value, "delay", translate("Delay in sec."))
answ_en = s:option(Flag, "answer_en", translate("Answer"))
answ = s:option(Value, "answer", translate("Answer MSG"))
delay:depends("delay_en", 1)
answ:depends("answer_en", 1)
s.addremove = true;
s.rmempty = true;
s.anonymous = true;

return m

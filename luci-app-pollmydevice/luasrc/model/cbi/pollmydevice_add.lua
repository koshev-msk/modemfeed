
local sys = require "luci.sys"
local uci = require "luci.model.uci".cursor()
local util = require ("luci.util")

local m = Map("pollmydevice", translate("PollMyDevice"))

local s = m:section(TypedSection, "interface", translate("TCP to RS232/RS485 converter"))
	s.addremove = true
	s.template = "pollmydevice/tblsection"
	s.novaluetext = translate("There are no PollMyDevice configurations yet")
	s.extedit = luci.dispatcher.build_url("admin", "services", "pollmydevice", "%s")
	s.defaults = {mode = "disabled"}
	s.sectionhead = "№"

o = s:option(DummyValue, "desc", translate("Description"))
	function o.cfgvalue(self, section)
		return self.map:get(section, self.option) or "-"
	end

o = s:option(DummyValue, "mode", translate("Mode"))
	function o.cfgvalue(self, section)
		return self.map:get(section, self.option) or "-"
	end

o = s:option(DummyValue, "devicename", translate("Port"))
	function o.cfgvalue(self, section)
		return self.map:get(section, self.option) or "-"
	end

o = s:option(DummyValue, "client_host", translate("Server Host or IP Address"))
	function o.cfgvalue(self, section)
		return self.map:get(section, self.option) or "-"
	end


o = s:option(DummyValue, "port", translate("Port"), translate("333"))
	function o.cfgvalue(self, section)
		local value = self.map:get(section, "client_port")
		if value then
			return value
		else
			value = self.map:get(section, "server_port")
			if value then
				return value
			else
				return "-"
			end
		end
	end

o = s:option(DummyValue, "client_auth", translate("Client Authentification"))
	function o.cfgvalue(self, section)
		local value = self.map:get(section, self.option) or "-"
		if value == "0" then
			return "disabled"
		else 
			if value == "1" then
				return "enabled"
			else
				return value
			end
		end
	end

o = s:option(DummyValue, "modbus_gateway", translate("Modbus TCP/IP"))
	function o.cfgvalue(self, section)
		local value = self.map:get(section, self.option) or "-"
		if value == "0" then
			return "disabled"
		else
			if value == "1" then
				return "RTU"
			else
				if value == "2" then
					return "ASCII"
				else
					return value
				end
			end
		end
	end

return m

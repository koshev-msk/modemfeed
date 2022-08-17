-------------------------------------------------------------
-- luci-app-gpoint. Gnss information dashboard for 3G/LTE dongle.
-------------------------------------------------------------
-- Copyright 2021-2022 Vladislav Kadulin <spanky@yandex.ru>
-- Licensed to the GNU General Public License v3.0

local fs   = require("nixio.fs")
local sys  = require("luci.sys")
local util = require("luci.util")
local json = require("luci.jsonc")


local packageName = "gpoint"
local helperText = ""
local tmpfsStatus, tmpfsStatusCode
local ubusStatus = util.ubus("service", "list", { name = packageName })
local lsusb = sys.exec("lsusb")
local device_port = fs.glob("/dev/tty[A-Z][A-Z]*")

local timezone = {
	{'Autodetect (experimental)','auto'},{'Etc/GMT',      '0' },{ 'Etc/GMT+1',   '1' },{ 'Etc/GMT+10',  '10'},
	{ 'Etc/GMT+11',  			   '11'},{ 'Etc/GMT+12',  '12'},{ 'Etc/GMT+2',   '2' },{ 'Etc/GMT+3',   '3' },
	{ 'Etc/GMT+4',   			   '4' },{ 'Etc/GMT+5',   '5' },{ 'Etc/GMT+6',   '6' },{ 'Etc/GMT+7',   '7' },
	{ 'Etc/GMT+8',  			   '8' },{ 'Etc/GMT+9',   '9' },{ 'Etc/GMT-1',  '-1' },{ 'Etc/GMT-10', '-10'},
	{ 'Etc/GMT-11',               '-11'},{ 'Etc/GMT-12', '-12'},{ 'Etc/GMT-13', '-13'},{ 'Etc/GMT-14', '-14'},
	{ 'Etc/GMT-2',                '-2' },{ 'Etc/GMT-3',  '-3' },{ 'Etc/GMT-4',  '-4' },{ 'Etc/GMT-5',  '-5' },
	{ 'Etc/GMT-6',                '-6' },{ 'Etc/GMT-7',  '-7' },{ 'Etc/GMT-8',  '-8' },{ 'Etc/GMT-9',  '-9' }
}

local modems = { 
	["2c7c:0306"] = "Quectel EP06",
	["2c7c:0512"] = "Quectel EM12",
	["2c7c:0125"] = "Quectel EC25",
	["2c7c:0800"] = "Quectel RM500Q"
}

local m = Map("gpoint", translate(""))

-- Service
local s = m:section(TypedSection, "modem_settings", translate("Service"))
s.anonymous = true
s.addremove = false

local o = s:option(DummyValue, "_dummy")
o.template = packageName .. "/buttons"

o = s:option(DummyValue, "_dummy", translate("Service Status:"))
o.template = packageName .. "/service_status"


-- Modem
s = m:section(TypedSection, "modem_settings", translate("Modem"), translate("Select the modem(s) to find the location"))
s.anonymous = true
s.addremove = false

local no_device = true
o = s:option(ListValue, "modem", translate("Modem(s):"))
if lsusb then
	for id, modem in pairs(modems) do
		if string.find(lsusb, id) then
			no_device = false
			local rename = string.gsub(modems[id], ' ', '_')
			o:value(rename, modems[id])
		end
	end
end

if no_device then
	o:value('mnf', translate("-- Modems not found --"))
end

o = s:option(ListValue, "port", translate("Modem port:"), translate("Select the NMEA port of the device."))
if no_device then
	o:value('pnf', translate("-- disable --"))
	o = s:option( DummyValue, "nfound")
	function o.cfgvalue(self, section)
		local nfound = "<div style=\"color: red;\"><b>No modem(s) found! Check the modem connections.</b><br> \
						<div style=\"color: lime;\">Supported modems: "
		for _, modem in pairs(modems) do
			nfound = nfound .. "<br>" .. "- " ..  modem
		end
		nfound = nfound .. "</div></div>"
		return translate(nfound)
	end
	o.rawhtml = true
else
	if device_port then
		for node in device_port do
			o:value(node, node)
		end
	end
end

-- Add TimeZone
o = s:option(ListValue, "timezone", translate("Timezone:"))
for _, zone in pairs(timezone) do
	o:value(zone[2], zone[1])
end


-- Remote Server
s = m:section(TypedSection, "server_settings", translate("Remote Server"), translate("Configuration of the remote navigation server"))
s.addremove = false
s.anonymous = true

o = s:option(Flag, "server_enable", translate("Enable server:"), translate("Enabling Remote Server service"))

o = s:option(ListValue, "proto", translate(" "), translate("Navigation data transmission protocol"))
o.widget = "radio"
o:value("traccar", " Traccar Client") -- Key and value pairs
o:value("wialon", " Wialon IPS")
o.default = "trackcar"

o = s:option(Value, "server_frequency", translate("Frequency:"), translate("Frequency of sending data to the Remote Server"))
o.placeholder = "In seconds"
o.datatype    = "range(5, 600)"

o = s:option(Value, "server_ip", translate("Address:"))
o.datatype = "host"
o.placeholder = '172.0.0.1'

o = s:option(Value, "server_port", translate("Port:"))
o.datatype = "port"
o.placeholder = '80'

o = s:option(Value, "server_login", translate("Login:"))
o.placeholder = "Device login (ID)"

o = s:option(Value, "server_password", translate("Password:"), translate("If you don't use Password, leave the field empty"))
o.password = true
o.placeholder = "Device password"

o = s:option(Flag, "blackbox_enable", translate("BlackBox enable:"), 
						translate("Blackbox makes it possible to record and store data even in the absence of a cellular signal"))
o:depends("proto","wialon")

o = s:option(Flag, "blackbox_cycle", translate("BlackBox cycle:"), translate("Cyclic overwriting of data stored in the BlackBox"))
o:depends("proto", "wialon")

o = s:option(Value, "blackbox_max_size", translate("BlackBox size:"), translate("Number of sentences in the BlackBox"))
o.placeholder = "default: 1000 sentence"
o.datatype    = "range(1000, 5000)"
o:depends("proto","wialon")

o = s:option(DummyValue, "_dummy", translate(" "))
o.template = packageName .. "/blackbox"
o:depends("proto","wialon")

o = s:option(Button, "clear", translate("Clear BlackBox"), translate("Warning! After clearing the BlackBox, GNSS data will be destroyed!"))
o.inputstyle = "remove"
o:depends("proto", "wialon")
function o.write(self, section)
	local file = io.open("/usr/share/gpoint/tmp/blackbox.json", 'w')
	file:write(json.stringify({["size"]=0,["max"]=1000,["data"]={}}))
    file:close()
end


-- Tab menu settings
s = m:section(TypedSection, "service_settings")
s.addremove = false
s.anonymous = true


----------------------------------------------------------------------------------------------------------------
s:tab("ya", translate("Yandex Locator"), translate("Determines the location of the mobile \
													device by the nearest Wi-Fi access points and \
													cellular base stations — without using satellite navigation systems."))
s:tab("gpoint_filter", translate("GeoHash Filter"), translate("Filters \"DRIFT\" and \"JUMPS\" of navigation 3G/LTE dongles"))
s:tab("kalman", translate("Kalman Filter"), translate("Designed to make the route smoother. Removes \"jumps\" of navigation 3G/LTE dongles"))

----------------------------------------------------------------------------------------------------------------

-- API Yandex locator 
o = s:taboption("ya", Flag, "ya_enable", translate("Enable:"), translate("Enabling the Yandex locator"))
o.optional = true

o = s:taboption("ya", ListValue, "ya_wifi", translate("Interface:"), translate("Select the Wi-Fi interface for Yandex locator"))
local iwinfo = sys.exec("iwinfo")
no_device = true
for device in string.gmatch(iwinfo, "(%S+)(%s%s%s%s%s)(%S+)") do
	o:value(device, device)
	no_device = false
end

if no_device then
	o:value('wnf', translate("-- Wifi not found --"))
end

o = s:taboption("ya", Value, "ya_key", translate("API Key:"), translate("To work with the Yandex locator must use an API key"))
o.password = true
o.placeholder = "Yandex API key"

o = s:taboption("ya", DummyValue, "ya_href")
	function o.cfgvalue(self, section)
		local h = "<a href=\"https://yandex.ru/dev/locator/keys/get/\">Get Yandex API key</a>"
		return translate(h)
	end
o.rawhtml = true

-- GeoHash
o = s:taboption("gpoint_filter", Flag, "filter_enable", translate("Enable:"), translate("Enabling GpointFilter"))
o.optional = true

o = s:taboption("gpoint_filter", Value, "filter_changes", translate("Jump:"), translate("Registration of the \"jump\" coordinates. \
		 The coordinate is recognized as valid after the modem has received it more than the specified number of times."))
o.placeholder = ""
o.datatype    = "range(2, 6)"

o = s:taboption("gpoint_filter", ListValue, "filter_hash", translate("Area:"), translate("The longer the hash length,\
			the smaller the area and the greater the accuracy of the coordinates in one area."))
o.optional = true
o.default = 7
for i = 1, 12 do
	o:value(i, i)
end
o = s:taboption("gpoint_filter", Value, "filter_speed", translate("Speed:"), translate("Above the specified speed, the filter will be disabled"))
o.placeholder = "default 2 km/h"
o.datatype    = "range(0, 150)"

-- Kalman
o = s:taboption("kalman", Flag, "kalman_enable", translate("Enable:"), translate("Enabling KalmanFilter"))
o.optional = true
o = s:taboption("kalman", Value, "kalman_noise", translate("Noise:"), translate("Noise is a parameter you can use to alter the expected noise.\
			1.0 is the original, and the higher it is, the more a path will be \"smoothed\""))
o.placeholder = ""
o.datatype    = "range(1.0, 30.0)"


return m
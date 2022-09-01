-- Copyright 2021 Konstantine Shevlyakov <shevlakov@132lan.ru>
-- Licensed to the GNU General Public License v3.0.

require("nixio.fs")
local m, d, s
local serial = "/dev/tty[A-Z][A-Z]*"
local qmi = "/dev/cdc-wdm*"
local uqmi = "/sbin/uqmi"
local libqmi = "/usr/bin/qmicli"
local mmcli = "/usr/bin/mmcli"
local gcom = "/usr/bin/comgt"
	

local m = Map("cellled", translate("CellLED: RSSI Cellular signal strength."),
	translate("<h3>General Setup<h3>"))

local mm = {}
local t = io.popen("mmcli -J -L | jsonfilter -e '@[\"modem-list\"][*]'", "r")
local d = m:section(TypedSection, "device")
d.anonymous = true
d.rmempty = true;
data = d:option(ListValue, "data_type", translate("Select Data service"))
local try_port = nixio.fs.glob(libqmi)
for node in try_port do
	data:value("qmi", "libqmi")
end
local try_port = nixio.fs.glob(uqmi)
for node in try_port do
	data:value("uqmi", "uqmi")
end
local try_port = nixio.fs.glob(mmcli)
for node in try_port do
	data:value("mm", "modemmanager")
end
local try_port = nixio.fs.glob(gcom)
for node in try_port do
	data:value("serial", "serial port")
end
devmm = d:option(ListValue, "device_mm", translate("Select Data port"))
if mm ~= nil then
	for devm in t:lines() do
		table.insert(mm, m)
		mm[#mm + 1] = devm
	end
	for b,g in ipairs(mm) do
		mm[b] = g
		if type(g) ~= "table" then
			n = io.popen
			devmm:value(g,g)
		end
	end
end
devmm:depends("data_type", "mm")
devq = d:option(ListValue, "device_qmi", translate("Select Data port"))
local try_port = nixio.fs.glob(qmi)
for node in try_port do
	devq:value(node, node)
end
devq:depends("data_type", "qmi")
devq:depends("data_type", "uqmi")
devs = d:option(ListValue, "device", translate("Select Data port"))
local try_port = nixio.fs.glob(serial)
for node in try_port do
        devs:value(node, node)
end
devs:depends("data_type", "serial")

timeout = d:option(Value, "timeout", translate("Timeout interval data"))
rgb = d:option(Flag, "rgb_led", translate("Use RGB Led"))
rgb.rmempty = true

pwm = d:option(Flag, "pwm_mode", translate("Use PWM"),
                translate("Enable if Support PWM LED"))
pwm:depends("rgb_led", 1)
local try_leds = nixio.fs.glob("/sys/class/leds/*")
red_led = d:option(ListValue, "red_led", translate("Red LED"))
if try_leds then
        local flash
        local status
        for flash in try_leds do
                local status = flash
                local flash = string.sub (status, 17)
                red_led:value(flash,flash)
        end
end

local try_leds = nixio.fs.glob("/sys/class/leds/*")
green_led = d:option(ListValue, "green_led", translate("Green LED"))
if try_leds then
        local flash
        local status
        for flash in try_leds do
                local status = flash
                local flash = string.sub (status, 17)
                green_led:value(flash,flash)
        end
end

local try_leds = nixio.fs.glob("/sys/class/leds/*")
blue_led = d:option(ListValue, "blue_led", translate("Blue LED"))
if try_leds then
        local flash
        local status
        for flash in try_leds do
                local status = flash
                local flash = string.sub (status, 17)
                blue_led:value(flash,flash)
        end
end
red_led:depends("rgb_led", 1)
green_led:depends("rgb_led", 1)
blue_led:depends("rgb_led", 1)

l = m:section(TypedSection, "device", "<p>&nbsp;</p>" .. translate("Signal strength values"))
l.anonymous = true

local s = m:section(TypedSection, "rssi_led")
rgb = s:option(Flag, "rgb", translate("RGB Led"))

local try_leds = nixio.fs.glob("/sys/class/leds/*")
led = s:option(ListValue, "led", translate("Select LED"))
if try_leds then
	local flash
	local status
	for flash in try_leds do
		local status = flash
		local flash = string.sub (status, 17)
		led:value(flash,flash)
	end
end
led:depends("rgb", 0)

quality = s:option(ListValue, "type", translate("Quality"))
quality:value("poor",translate("Poor"))
quality:value("bad",translate("Bad"))
quality:value("fair",translate("Fair"))
quality:value("good",translate("Good"))
quality:depends("rgb", 1)
rssi_min = s:option(Value, "rssi_min", translate("Min.value \"%\""))
rssi_max = s:option(Value, "rssi_max", translate("Max.value \"%\""))
s.addremove = true;
s.rmempty = true;
s.anonymous = true;

return m

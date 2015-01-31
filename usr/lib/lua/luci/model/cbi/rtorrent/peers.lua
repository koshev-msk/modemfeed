--[[
LuCI - Lua Configuration Interface - rTorrent client

Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

local rtorrent = require "rtorrent"
local http = require "socket.http"
local common = require "luci.model.cbi.rtorrent.common"

local hash = luci.dispatcher.context.requestpath[4]
local details = rtorrent.batchcall(hash, "d.", {"name"})
local format, map = {}, {}

function map.googlemap(latitude, longitude, zoom)
	return "https://google.com/maps/place/%s,%s/@%s,%s,%sz" % {latitude, longitude, latitude, longitude, zoom}
end

function map.openstreetmap(latitude, longitude, zoom)
	return "http://www.openstreetmap.org/?mlat=%s&mlon=%s#map=%s/%s/%s/m" % {latitude, longitude, zoom, latitude, longitude}
end

function format.address(r, v)
	local map = map.googlemap(r.latitude, r.longitude, 11)
	-- local map = map.openstreetmap(r.latitude, r.longitude, 11)
	local flag = "<img src=\"http://www.iplocation.net/images/flags/%s.gif\" />" % string.lower(r["country_code"])
	return "%s <a href=\"%s\" style=\"color: #404040;\" target=\"_blank\">%s</a>" % {flag, map, v}
end

function format.completed_percent(r, v)
	return own.html(string.format("%.1f%%", v), "center")
end

function format.client_version(r, v)
	return own.html(v, "center")
end

function format.down_rate(d, v)
	return own.html(string.format("%.2f", v / 1000), "center")
end

function format.up_rate(d, v)
	return own.html(string.format("%.2f", v / 1000), "center")
end

function format.down_total(d, v)
	return own.html(own.human_size(v), "nowrap", "center", "title: " .. v .. " B")
end

function format.up_total(d, v)
	return own.html(own.human_size(v), "nowrap", "center", "title: " .. v .. " B")
end

function json2table(json)
	loadstring("j2t = " .. string.gsub(string.gsub(json, '([,%{])%s*\n?%s*"', '%1["'), '"%s*:%s*', '"]='))()
	return j2t
end

function format.location(r, v)
	return own.html(v, "center")
end

function ip2geo(ip)
	-- return http.request("http://www.geoplugin.net/json.gp?ip=%s" % ip)
	return http.request("http://www.telize.com/geoip/%s" % ip)
end

function add_location(r)
	for i, j in pairs(json2table(ip2geo(r.address))) do
		r[i] = j
	end
	local location = {}
	for _, k in ipairs({"country", "region", "city"}) do
		if r[k] ~= "" then table.insert(location, r[k]) end
	end
	r["location"] = table.concat(location, "/")
end

local list = rtorrent.multicall("p.", hash, 0, "address", "completed_percent", "client_version", 
	"down_rate", "up_rate", "up_total", "down_total")

for _, r in ipairs(list) do
	add_location(r)
	for k, v in pairs(r) do
		if format[k] then
			r[k] = format[k](r, v)
		end
	end
end

f = SimpleForm("rtorrent", details["name"])
f.redirect = luci.dispatcher.build_url("admin/rtorrent/main")
f.reset = false
f.submit = false

t = f:section(Table, list)
t.template = "rtorrent/list"
t.pages = common.get_pages(hash)
t.page = "peer list"

t:option(DummyValue, "address", own.html("Address", "title: Peer IP address")).rawhtml = true
t:option(DummyValue, "client_version", own.html("Client", "center", "title: Client version")).rawhtml = true
t:option(DummyValue, "location", own.html("Location", "center", "title: Location: country/region/city")).rawhtml = true
t:option(DummyValue, "completed_percent", own.html("Done", "center", "title: Download done percent")).rawhtml = true
t:option(DummyValue, "down_rate", own.html("Down<br />speed", "center", "title: Download speed in kb/s")).rawhtml = true
t:option(DummyValue, "up_rate", own.html("Up<br />speed", "center", "title: Upload speed in kb/s")).rawhtml = true
t:option(DummyValue, "down_total", own.html("Downloaded", "center", "title: Total downloaded")).rawhtml = true
t:option(DummyValue, "up_total", own.html("Uploaded", "center", "title: Total uploaded")).rawhtml = true

return f


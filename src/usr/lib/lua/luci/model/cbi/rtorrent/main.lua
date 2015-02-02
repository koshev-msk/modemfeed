--[[
LuCI - Lua Configuration Interface - rTorrent client

Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

local common = require "luci.model.cbi.rtorrent.common"
local rtorrent = require "rtorrent"
local own = require "own"

local selected, format = {}, {}
local total = {["name"] = 0, ["size_bytes"] = 0, ["down_rate"] = 0, ["up_rate"] = 0}

local methods = { "hash", "name", "size_bytes", "done_percent", "status", "eta", "icon",
	 "peers_accounted", "peers_complete", "down_rate", "up_rate", "ratio", "up_total", "custom2" }

function has_tag(tags, tag)
	for _, t in ipairs(tags) do
		if t.name == tag then return true end
	end
	return false
end

function get_tags(details)
	local l = {}
	for _, d in ipairs(details) do
		for p in string.gmatch(d["custom2"] or "all", "%S+") do
			if not has_tag(l, p) then
				table.insert(l, {name = p, link = luci.dispatcher.build_url("admin/rtorrent/main/%s" % p)})
			end
		end
	end
	return l
end

function filter(details, page)
	local filtered = {}
	for _, d in ipairs(details) do
		if string.find(d["custom2"] or "all", page) then
			table.insert(filtered, d)
		end
	end
	return filtered
end

local details = rtorrent.multicall("d.", "default", unpack(methods))
local tags = get_tags(details)
local user = luci.dispatcher.context.authuser
local page = luci.dispatcher.context.requestpath[4] or (has_tag(tags, user) and user or "all")
local filtered = filter(details, page)

function format.icon(d, v)
	return "<img src=\"" .. v .. "\" />"
end

function format.name(d, v)
	total["name"] = total["name"] + 1
	local url = luci.dispatcher.build_url("admin/rtorrent/files/" .. d["hash"])
	return "<div style=\"word-break: break-all\"><a href=\"%s\" style=\"color: #404040;\">%s</a></div>" % {url, v}
end

function format.size_bytes(d, v)
	total["size_bytes"] = total["size_bytes"] + v
	return own.html(common.human_size(v), "nowrap", "center", "title: " .. v .. " B")
end

function format.done_percent(d, v)
	return own.html(string.format("%.1f%%", v), "center")
end

function format.status(d, v)
	return own.html(v, "center", v == "close" and "red", v == "seed" and "blue",
		v == "down" and "green", v == "hash" and "green")
end

function format.peers_accounted(d, v)
	return own.html(tostring(v), "center")
end

function format.peers_complete(d, v)
	return own.html(tostring(v), "center")
end

function format.down_rate(d, v)
	total["down_rate"] = total["down_rate"] + v
	return own.html(string.format("%.2f", v / 1000), "center")
end

function format.up_rate(d, v)
	total["up_rate"] = total["up_rate"] + v
	return own.html(string.format("%.2f", v / 1000), "center")
end

function format.ratio(d, v)
	return own.html(string.format("%.2f", v / 1000), "center", v < 1000 and "red" or "green",
		"title: Total uploaded: " .. common.human_size(d["up_total"]))
end

function format.eta(d, v)
	-- return own.html(common.human_size(tonumber(v)), "nowrap", "center")
	return type(v) == "number" and common.human_time(v) or v
end

function add_summary(details)
 	table.insert(details, {
 		["name"] = own.html("TOTAL: " .. total["name"] .. " pcs.", "bold"),
 		["size_bytes"] = own.html(common.human_size(total["size_bytes"]), "bold", "center"),
 		["down_rate"] =  own.html(string.format("%.2f", total["down_rate"] / 1000), "bold", "center"),
 		["up_rate"] =  own.html(string.format("%.2f", total["up_rate"] / 1000), "bold", "center"),
 		["select"] = "hidden"
 	})
	return details
end

function html_format(details)
	table.sort(details, function(a, b) return a["name"] < b["name"] end)
	for _, d in ipairs(details) do
		for m, v in pairs(d) do
			d[m] = format[m] and format[m](d, v) or v
		end
	end
	-- return add_summary(details)
	return details
end

f = SimpleForm("rtorrent")
f.reset = false
f.submit = false

t = f:section(Table, html_format(filtered))
t.template = "rtorrent/list"
t.pages = tags
t.page = page
t:option(DummyValue, "icon").rawhtml = true
t:option(DummyValue, "name", "Name").rawhtml = true
t:option(DummyValue, "size_bytes", own.html("Size", "center", "title: Full size of torrent")).rawhtml = true
t:option(DummyValue, "done_percent", own.html("Done", "center", "title: Download done percent")).rawhtml = true
t:option(DummyValue, "status", own.html("Status", "center")).rawhtml = true
t:option(DummyValue, "peers_accounted", own.html("&uarr;", "center", "title: Seeder count")).rawhtml = true
t:option(DummyValue, "peers_complete", own.html("&darr;", "center", "title: Leecher count")).rawhtml = true
t:option(DummyValue, "down_rate", own.html("Down<br />speed", "center", "title: Download speed in kb/s")).rawhtml = true
t:option(DummyValue, "up_rate", own.html("Up<br />speed", "center", "title: Upload speed in kb/s")).rawhtml = true
t:option(DummyValue, "ratio", own.html("Ratio", "center", "title: Download/upload ratio")).rawhtml = true
t:option(DummyValue, "eta", own.html("ETA", "center", "title: Estimated Time of Arrival")).rawhtml = true
select = t:option(Flag, "select", "")

function select.write(self, section, value)
	table.insert(selected, filtered[section].hash)
end

s = f:section(SimpleSection)
s.template = "rtorrent/buttonsection"
s.style = "float: right;"

start = s:option(Button, "start", "start")
start.template = "rtorrent/button"
start.inputstyle = "apply"

function start.write(self, section, value)
	if next(selected) ~= nil then
		for _, hash in ipairs(selected) do
			rtorrent.call("d.open", hash)
			rtorrent.call("d.start", hash)
		end
		luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/main"))
	end
end

close = s:option(Button, "close", "close")
close.template = "rtorrent/button"
close.inputstyle = "reset"

function close.write(self, section, value)
	if next(selected) ~= nil then
		for _, hash in ipairs(selected) do
			rtorrent.call("d.close", hash)
		end
		luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/main"))
	end
end

delete = s:option(Button, "delete", "delete")
delete.template = "rtorrent/button"
delete.inputstyle = "remove"

function delete.write(self, section, value)
	if next(selected) ~= nil then
		for _, hash in ipairs(selected) do
			rtorrent.call("d.close", hash)
			rtorrent.call("d.erase", hash)
		end
		luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/main"))
	end
end

return f


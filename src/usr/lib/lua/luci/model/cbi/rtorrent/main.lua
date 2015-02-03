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

local selected, format, total = {}, {}, {}

local methods = { "hash", "name", "size_bytes", "bytes_done", "hashing", "open", "state", "complete",
	"peers_accounted", "peers_complete", "down_rate", "up_rate", "ratio", "up_total", "custom1", "custom2" }

function status(d)
	if     d["hashing"] > 0 then return "hash"
	elseif d["open"] == 0 then return "close"
	elseif d["state"] == 0 then return "stop"
	elseif d["state"] > 0 then
		if d["complete"] == 0 then return "down"
		else return "seed" end
	else return "unknown" end
end

function eta(d)
	if d["bytes_done"] < d["size_bytes"] then
		if d["down_rate"] > 0 then
			return common.human_time((d["size_bytes"] - d["bytes_done"]) / d["down_rate"])
		else return "&#8734;" end
	else return "--" end
end

function favicon(d)
	if not d["custom1"] or d["custom1"] == "" then
		d["custom1"] = "/luci-static/resources/icons/unknown_tracker.png"
		for _, t in pairs(rtorrent.multicall("t.", d["hash"], 0, "url", "enabled")) do
			if t["enabled"] then
				local domain = t["url"]:match("[%w%.:/]*[%./](%w+%.%w+)")
				if domain then
					local icon = "http://" .. domain .. "/favicon.ico"
					if common.get(icon) then
						d["custom1"] = icon
						break
					end
				end
			end
		end
		rtorrent.call("d.set_custom1", d["hash"], d["custom1"])
	end
 	return d["custom1"]
end

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

function format.icon(d, v)
	return "<img src=\"" .. v .. "\" />"
end

function format.name(d, v)
	total["name"] = (total["name"] or 0) + 1
	local url = luci.dispatcher.build_url("admin/rtorrent/files/" .. d["hash"])
	return "<a href=\"%s\">%s</a>" % {url, v}
end

function format.size_bytes(d, v)
	total["size_bytes"] = (total["size_bytes"] or 0) + v
	return "<div title=\"%s B\">%s</div>" % {v, common.human_size(v)}
end

function format.done_percent(d, v)
	return string.format("%.1f%%", v)
end

function format.status(d, v)
	return common.div(v, v == "close" and "red", v == "seed" and "blue",
		v == "down" and "green", v == "hash" and "green")
end

function format.down_rate(d, v)
	total["down_rate"] = (total["down_rate"] or 0) + v
	return string.format("%.2f", v / 1000)
end

function format.up_rate(d, v)
	total["up_rate"] = (total["up_rate"] or 0) + v
	return string.format("%.2f", v / 1000)
end

function format.ratio(d, v)
	return common.div(string.format("%.2f", v / 1000), v < 1000 and "red" or "green")
	--	"title: Total uploaded: " .. common.human_size(d["up_total"]))
end

function add_custom(details)
	for _, d in ipairs(details) do
		d["status"] = status(d)
		d["done_percent"] = d["bytes_done"] * 100.0 / d["size_bytes"]
		d["eta"] = eta(d)
		d["icon"] = favicon(d)
	end
end

function add_summary(details)
 	table.insert(details, {
 		["name"] = "TOTAL: " .. total["name"] .. " pcs.",
 		["size_bytes"] = common.human_size(total["size_bytes"]),
 		["down_rate"] = string.format("%.2f", total["down_rate"] / 1000),
 		["up_rate"] =  string.format("%.2f", total["up_rate"] / 1000),
 		["select"] = "%hidden%"
 	})
end

function html_format(details)
	table.sort(details, function(a, b) return a["name"] < b["name"] end)
	for _, d in ipairs(details) do
		for m, v in pairs(d) do
			d[m] = format[m] and format[m](d, v) or tostring(v)
		end
	end
end

f = SimpleForm("rtorrent")
f.reset = false
f.submit = false

local details = rtorrent.multicall("d.", "default", unpack(methods))
add_custom(details)
local tags = get_tags(details)
local user = luci.dispatcher.context.authuser
local page = luci.dispatcher.context.requestpath[4] or (has_tag(tags, user) and user or "all")
local filtered = filter(details, page)

html_format(filtered)
if #filtered > 1 then add_summary(filtered) end
t = f:section(Table, filtered)
t.template = "rtorrent/list"
t.pages = tags
t.page = page
t.headcol = 2

AbstractValue.tooltip = function(self, s) self.hint = s return self end

t:option(DummyValue, "icon").rawhtml = true
t:option(DummyValue, "name", "Name").rawhtml = true
t:option(DummyValue, "size_bytes", "Size"):tooltip("Full size of torrent").rawhtml = true
t:option(DummyValue, "done_percent", "Done"):tooltip("Download done percent").rawhtml = true
t:option(DummyValue, "status", "Status").rawhtml = true
t:option(DummyValue, "peers_accounted", "&uarr;"):tooltip("Seeder count").rawhtml = true
t:option(DummyValue, "peers_complete", "&darr;"):tooltip("Leecher count").rawhtml = true
t:option(DummyValue, "down_rate", "Down<br />speed"):tooltip("Download speed in kb/s").rawhtml = true
t:option(DummyValue, "up_rate", "Up<br />speed"):tooltip("Upload speed in kb/s").rawhtml = true
t:option(DummyValue, "ratio", "Ratio"):tooltip("Download/upload ratio").rawhtml = true
t:option(DummyValue, "eta", "ETA"):tooltip("Estimated Time of Arrival").rawhtml = true
select = t:option(Flag, "select")
select.template = "rtorrent/fvalue"

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


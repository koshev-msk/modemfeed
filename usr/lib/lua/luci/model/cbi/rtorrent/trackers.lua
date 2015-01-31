--[[
LuCI - Lua Configuration Interface - rTorrent client

Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

local own = require "own"
local rtorrent = require "rtorrent"
local common = require "luci.model.cbi.rtorrent.common"

local hash = luci.dispatcher.context.requestpath[4]
local details = rtorrent.batchcall(hash, "d.", {"name"})
local format = {}

function format.enabled(r, v)
	return tostring(v)
end

function format.scrape_complete(r, v)
	return own.html(tostring(v), "center")
end

function format.scrape_incomplete(r, v)
	return own.html(tostring(v), "center")
end

function format.scrape_downloaded(r, v)
	return own.html(tostring(v), "center")
end

function format.scrape_time_last(r, v)
	return own.html(own.human_time(os.time() - v), "center")
end

local list = rtorrent.multicall("t.", hash, 0, "enabled", "url", "scrape_downloaded", "scrape_complete",
	"scrape_incomplete", "scrape_time_last")

for _, r in ipairs(list) do
	for k, v in pairs(r) do
		if format[k] then
			r[k] = format[k](r, v)
		end
	end
end

f = SimpleForm("rtorrent", details["name"])
f.redirect = luci.dispatcher.build_url("admin/rtorrent/main")

t = f:section(Table, list)
t.template = "rtorrent/list"
t.pages = common.get_pages(hash)
t.page = "tracker list"

enabled = t:option(Flag, "enabled", "Enabled")
enabled.rmempty = false
enabled.rawhtml = true

function enabled.write(self, section, value)
	if value ~= tostring(list[section].enabled) then
		rtorrent.call("t.set_enabled", hash, section - 1, tonumber(value))
		luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/trackers/%s" % hash))
	end
end

t:option(DummyValue, "url", "Url").rawhtml = true
t:option(DummyValue, "scrape_downloaded", own.html("D", "center", "title: Downloaded")).rawhtml = true
t:option(DummyValue, "scrape_complete", own.html("S", "center", "title: Seeders")).rawhtml = true
t:option(DummyValue, "scrape_incomplete", own.html("L", "center", "title: Leechers")).rawhtml = true
t:option(DummyValue, "scrape_time_last", own.html("Updated", "center", "title: Last update time")).rawhtml = true

add = f:field(Value, "add_tracker", "Add tracker")
function add.write(self, section, value)
	rtorrent.call("d.tracker.insert", hash, table.getn(list), value)
	luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/trackers/%s" % hash))
end

return f


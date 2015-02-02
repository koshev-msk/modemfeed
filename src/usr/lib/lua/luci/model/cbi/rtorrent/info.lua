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
local common = require "luci.model.cbi.rtorrent.common"

local hash = luci.dispatcher.context.requestpath[4]
local details = rtorrent.batchcall(hash, "d.", {"name", "custom2"})

f = SimpleForm("rtorrent", details["name"])
f.redirect = luci.dispatcher.build_url("admin/rtorrent/main")

t = f:section(Table, list)
t.template = "rtorrent/list"
t.pages = common.get_pages(hash)
t.page = "info"

h = f:field(DummyValue, "hash", "Hash")
function h.cfgvalue(self, section)
	return hash
end

tags = f:field(Value, "tags", "Tags")
tags.default = details["custom2"]
tags.rmempty = false

function tags.write(self, section, value)
	rtorrent.call("d.set_custom2", hash, value)
end

function f.handle(self, state, data)    
	if state == FORM_VALID then
		luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/info/") .. hash)
	end
	return true
end

return f


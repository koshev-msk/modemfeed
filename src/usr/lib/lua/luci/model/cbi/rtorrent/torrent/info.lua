-- Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>
-- Licensed to the public under the Apache License 2.0.

local rtorrent = require "rtorrent"
local common = require "luci.model.cbi.rtorrent.common"

local hash = arg[1]
local details = rtorrent.batchcall({"name", "custom2"}, hash, "d.")

f = SimpleForm("rtorrent", details["name"])
f.redirect = luci.dispatcher.build_url("admin/rtorrent/main")

t = f:section(Table, list)
t.template = "rtorrent/list"
t.pages = common.get_torrent_pages(hash)
t.page = "Info"

h = f:field(DummyValue, "hash", "Hash")
function h.cfgvalue(self, section)
	return hash
end

tags = f:field(Value, "tags", "Tags")
tags.default = details["custom2"]
tags.rmempty = false

function tags.write(self, section, value)
	rtorrent.call("d.custom2.set", hash, value)
end

function f.handle(self, state, data)    
	if state == FORM_VALID then
		luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/info/") .. hash)
	end
	return true
end

return f


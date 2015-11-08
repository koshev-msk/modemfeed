-- Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>
-- Licensed to the public under the Apache License 2.0.

local common = require "luci.model.cbi.rtorrent.common"

m = Map("rtorrent", "Admin - RSS Downloader")

s = m:section(TypedSection, "rss-feed")
s.addremove = true
s.anonymous = true
s.sortable = true
s.template = "cbi/tblsection"
s.render = function(self, section, scope)
	luci.template.render("rtorrent/tabmenu", { self = {
		pages = common.get_admin_pages(),
		page = "RSS"
	}})
	TypedSection.render(self, section, scope)
end

name = s:option(Value, "name", "Name")
name.rmempty = false

url = s:option(Value, "url", "RSS Feed URL")
url.size = "75px"
url.rmempty = false

enabled = s:option(Flag, "enabled", "Enabled")
enabled.rmempty = false

return m


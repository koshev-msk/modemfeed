-- Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>
-- Licensed to the public under the Apache License 2.0.

local common = require "luci.model.cbi.rtorrent.common"

f = SimpleForm("rtorrent", "Admin - rTorrent")
-- f.redirect = luci.dispatcher.build_url("admin/rtorrent/main")

-- dir = f:field(DummyValue, "dummy", luci.dispatcher.context.authuser)

t = f:section(Table, list)
t.template = "rtorrent/list"
t.pages = common.get_admin_pages()
t.page = "rTorrent"

-- s = f:section(TypedSection, "watch")
-- s.addremove = true
-- s.anonymous = true
-- s.sortable = true
-- s.template = "rtorrent/list"
-- s.pages = common.get_admin_pages()
-- s.page = "rTorrent"

return f


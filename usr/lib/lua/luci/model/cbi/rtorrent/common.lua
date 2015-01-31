--[[
LuCI - Lua Configuration Interface - rTorrent client

Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

local luci = require "luci"

module "luci.model.cbi.rtorrent.common"

function get_pages(hash)
	return {
		{ name = "info", link = luci.dispatcher.build_url("admin/rtorrent/info/") .. hash },
		{ name = "file list", link = luci.dispatcher.build_url("admin/rtorrent/files/") .. hash },
		{ name = "tracker list", link = luci.dispatcher.build_url("admin/rtorrent/trackers/") .. hash },
		{ name = "peer list", link = luci.dispatcher.build_url("admin/rtorrent/peers/") .. hash }
	}
end


-- Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>
-- Licensed to the public under the Apache License 2.0.

local nixio = require "nixio"
local dm = require "luci.model.cbi.rtorrent.download"

module("luci.controller.rtorrent", package.seeall)

function index()
	entry({"admin", "rtorrent"},  firstchild(), "Torrent", 45).dependent = false
	entry({"admin", "rtorrent", "main"}, cbi("rtorrent/main"), "Main", 10).leaf = true
	entry({"admin", "rtorrent", "add"}, cbi("rtorrent/add", {autoapply=true}), "Add", 20)
	entry({"admin", "rtorrent", "admin"}, cbi("rtorrent/admin"), "Admin", 30)
	entry({"admin", "rtorrent", "watch"}, cbi("rtorrent/watch"), "Watch", 40)

	entry({"admin", "rtorrent", "files"}, cbi("rtorrent/files"), nil).leaf = true
	entry({"admin", "rtorrent", "trackers"}, cbi("rtorrent/trackers"), nil).leaf = true
	entry({"admin", "rtorrent", "peers"}, cbi("rtorrent/peers"), nil).leaf = true
	entry({"admin", "rtorrent", "info"}, cbi("rtorrent/info"), nil).leaf = true

	entry({"admin", "rtorrent", "download"}, call("download"), nil).leaf = true
	entry({"admin", "rtorrent", "downloadall"}, call("downloadall"), nil).leaf = true
end

function download()
	dm.download_file(nixio.bin.b64decode(luci.dispatcher.context.requestpath[4]))
end

function downloadall()
	dm.download_all(nixio.bin.b64decode(luci.dispatcher.context.requestpath[4]))
end


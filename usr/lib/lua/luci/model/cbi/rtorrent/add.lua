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
local bencode = require "bencode"
local nixio = require "nixio"
local rtorrent = require "rtorrent"
local xmlrpc = require "xmlrpc"

f = SimpleForm("rtorrent", "Add torrent")

local torrent

uri = f:field(TextValue, "uri", "Torrent<br />or magnet URI")
uri.rows = 1

function uri.validate(self, value, section)
	if "magnet:" == string.sub(own.trim(value), 1, 7) then
		torrent = bencode.encode({ ["magnet-uri"] = own.trim(value) })
	else
		local ok, res = own.get(value)
		if not ok then return nil, "Not able to download torrent: " .. res end
		local tab, err = bencode.decode(res)
		if not tab then return nil, "Not able to parse torrent file: " .. err end
		torrent = res
	end
	return value
end

file = f:field(FileUpload, "file", "Upload torrent file")

function file.validate(self, value, section)
	torrent = nixio.fs.readfile(value)
	self:remove(section)
	local tab, err = bencode.decode(torrent)
	if not tab then return nil, "Not able to parse torrent file: " .. err end
	return value
end

dir = f:field(Value, "dir", "Download directory")
dir.default = "/store/download"
dir.datatype = "directory"
dir.rmempty = false

labels = f:field(Value, "labels", "Labels")
labels.default = "all " .. luci.dispatcher.context.authuser
labels.rmempty = false

start = f:field(Flag, "start", "Start now")
start.default  = "1"
start.rmempty  = false

function f.handle(self, state, data)
	if state == FORM_VALID and torrent and #torrent > 0 then
		local params = {}
		table.insert(params, data.start == "1" and "load_raw_start" or "load_raw")
		table.insert(params, xmlrpc.newTypedValue((nixio.bin.b64encode(torrent)), "base64"))
		table.insert(params, "d.set_directory=\"" .. data.dir .. "\"")
		table.insert(params, "d.set_custom2=\"" .. data.labels .. "\"")
		if data.uri then
			table.insert(params, "d.set_custom3=" .. nixio.bin.b64encode(data.uri))
		end
		rtorrent.call(unpack(params))
		luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/add"))
	end
	return true
end

return f


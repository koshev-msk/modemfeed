--[[
LuCI - Lua Configuration Interface - rTorrent client

Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

local bencode = require "bencode"
local nixio = require "nixio"
local rtorrent = require "rtorrent"
local xmlrpc = require "xmlrpc"
local common = require "luci.model.cbi.rtorrent.common"

f = SimpleForm("rtorrent", "Add torrent")

local torrent

uri = f:field(TextValue, "uri", "Torrent<br />or magnet URI")
uri.rows = 1

function trim(s)
	return s:match('^%s*(.*%S)') or ''
end

function uri.validate(self, value, section)
	if "magnet:" == string.sub(trim(value), 1, 7) then
		torrent = bencode.encode({ ["magnet-uri"] = trim(value) })
	else
		local ok, res = common.get(value)
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
dir.default = rtorrent.call("get_directory")
dir.datatype = "directory"
dir.rmempty = false

tags = f:field(Value, "tags", "Tags")
local user =  luci.dispatcher.context.authuser
tags.default = "all" .. (user ~= "root" and " " .. user or "")
tags.rmempty = false

start = f:field(Flag, "start", "Start now")
start.default  = "1"
start.rmempty  = false

function f.handle(self, state, data)
	if state == FORM_VALID and torrent and #torrent > 0 then
		local params = {}
		table.insert(params, data.start == "1" and "load_raw_start" or "load_raw")
		table.insert(params, xmlrpc.newTypedValue((nixio.bin.b64encode(torrent)), "base64"))
		table.insert(params, "d.set_directory=\"" .. data.dir .. "\"")
		table.insert(params, "d.set_custom2=\"" .. data.tags .. "\"")
		if data.uri then
			table.insert(params, "d.set_custom3=" .. nixio.bin.b64encode(data.uri))
		end
		rtorrent.call(unpack(params))
		luci.http.redirect(luci.dispatcher.build_url("admin/rtorrent/add"))
	end
	return true
end

return f


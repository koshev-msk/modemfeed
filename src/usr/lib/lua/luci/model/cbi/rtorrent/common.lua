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
local cURL = require "cURL"

local string, os, math, ipairs, table, pcall = string, os, math, ipairs, table, pcall

module "luci.model.cbi.rtorrent.common"

function get_pages(hash)
	return {
		{ name = "info", link = luci.dispatcher.build_url("admin/rtorrent/info/") .. hash },
		{ name = "file list", link = luci.dispatcher.build_url("admin/rtorrent/files/") .. hash },
		{ name = "tracker list", link = luci.dispatcher.build_url("admin/rtorrent/trackers/") .. hash },
		{ name = "peer list", link = luci.dispatcher.build_url("admin/rtorrent/peers/") .. hash }
	}
end

function human_size(bytes)
	local symbol = {[0]="B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"}
	local exp = bytes > 0 and math.floor(math.log(bytes) / math.log(1024)) or 0
	local value = bytes / math.pow(1024, exp)
	local acc = bytes > 0 and 2 - math.floor(math.log10(value)) or 2
	if acc < 0 then acc = 0 end
	return string.format("%." .. acc .. "f " .. symbol[exp], value)
end

function human_time(sec)
	local t = os.date("*t", sec)
	if     t["day"]  > 25 then return "&#8734;"
	elseif t["day"]  > 1 then return string.format("%dd<br />%dh %dm", t["day"] - 1, t["hour"], t["min"])
	elseif t["hour"] > 1 then return string.format("%dh<br />%dm %ds", t["hour"] - 1, t["min"], t["sec"])
	elseif t["min"] > 0 then return string.format("%dm %ds", t["min"], t["sec"])
	else   return string.format("%ds", t["sec"]) end
end

function div(body, ...)
	local class = {}
	for _, c in ipairs({...}) do
		if c then table.insert(class, c) end
	end
	if #class > 0 then
		return "<div class=\"%s\">%s</div>" % {table.concat(class, " "), body}
	else
		return body
	end
end

function get(url)
	local function fetch(url)
		local res = {}
		local res_code, res_message = "500", ""
	
		curl = cURL.easy_init()
		curl:setopt_url(url)
		curl:setopt_header(0)
		curl:setopt_timeout(5)
		curl:setopt_useragent("unknown")
		curl:setopt_referer("http://www.google.com")
		curl:setopt_ssl_verifypeer(0)
		curl:setopt_ssl_verifyhost(0)
		curl:setopt_followlocation(1)
		curl:setopt_encoding("")
		curl:setopt_maxredirs(10)
		curl:setopt_cookiefile("/etc/cookies.conf")
		curl:perform({
			headerfunction = function(s)
				local code, message = string.match(s, "^HTTP/.* (%d+) (.+)$")
				if code then res_code, res_message = code, message end
			end,
			writefunction = function(buf) table.insert(res, buf) end
		})		
	 	if res_code == "200" then return true, table.concat(res)
		else return false, res_code .. ": " .. res_message end
	end

	local ok, r1, r2 = pcall(fetch, url)
	if ok then return r1, r2 else return ok, r1 end
end


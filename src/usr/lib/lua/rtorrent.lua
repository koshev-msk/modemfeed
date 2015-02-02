--[[
rTorrent XML-RPC helper library

Copyright 2014-2015 Sandor Balazsi <sandor.balazsi@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

local pairs, ipairs, string, tostring, table = pairs, ipairs, string, tostring, table
local assert, type, unpack = assert, type, unpack

local xmlrpc = require "xmlrpc"
local scgi = require "xmlrpc.scgi"
local own = require "own"

local scgi_address = "localhost"
local scgi_port = 5000

module "rtorrent"

function favicon(d)
	if not d["custom1"] or d["custom1"] == "" then
		d["custom1"] = "/luci-static/resources/icons/unknown_tracker.png"
		for _, t in pairs(multicall("t.", d["hash"], 0, "url", "enabled")) do
			if t["enabled"] then
				local domain = t["url"]:match("[%w%.:/]*[%./](%w+%.%w+)")
				own.log("url: " .. t["url"] .. " domain: " .. domain)
				if domain then
					local icon = "http://" .. domain .. "/favicon.ico"
					if own.get(icon) then
						d["custom1"] = icon
						break
					end
				end
			end
		end
		call("d.set_custom1", d["hash"], d["custom1"])
	end
 	return d["custom1"]
end

function status(d)
	if     d["hashing"] > 0 then return "hash"
	elseif d["open"] == 0 then return "close"
	elseif d["state"] == 0 then return "stop"
	elseif d["state"] > 0 then
		if d["complete"] == 0 then return "down"
		else return "seed" end
	else return "unknown" end
end

function eta(d)
	if d["bytes_done"] < d["size_bytes"] then
		if d["down_rate"] > 0 then return (d["size_bytes"] - d["bytes_done"]) / d["down_rate"]
		else return "&#8734;" end
	else return "--" end
end

function accessor(prefix, methods, postfix)
	methods = own.map(methods, function(m)
		if m == 0 then return m end
		local acc = "get_"
		local is_methods = {
			"active", "hash_checked", "hash_checking", "multi_file", "not_partially_done",		-- (d)etails
			"open", "partially_done", "pex_active", "private",
			"create_queued", "created", "open", "resize_queued",					-- (f)iles
			"encrypted", "incoming", "obfuscated", "preferred", "snubbed", "unwanted",	 	-- (p)eers
			"busy", "enabled", "enabled.set", "extra_tracker", "open", "usable"			-- (t)rackers
		}
		for i = 1, #is_methods do
			if m == is_methods[i] then
				acc = "is_"
				break
			end
		end
		acc = acc .. m
		if prefix then acc = prefix .. acc end
		if postfix then acc = acc .. postfix end
		return acc
	end)
	return methods
end

function depends(methods)
	local deps = {}
	for _, m in pairs(methods) do
	        if     m == "icon" then deps = own.merge(deps, {"hash", "custom1"})
		elseif m == "done_percent" then deps = own.merge(deps, {"size_bytes", "bytes_done"})
		elseif m == "chunks_percent" then deps = own.merge(deps, {"size_chunks", "completed_chunks"})
		elseif m == "status" then deps = own.merge(deps, {"hashing", "open", "state", "complete"})
		elseif m == "eta" then deps = own.merge(deps, {"size_bytes", "bytes_done", "down_rate"})
		else   deps = own.merge(deps, {m}) end
	end
	return deps
end

function customize(res, methods)
	for _, r in ipairs(res) do
		for _, m in pairs(methods) do
			if     m == "icon" then r[m] = favicon(r)
			elseif m == "done_percent" then r[m] = r["bytes_done"] * 100.0 / r["size_bytes"]
			elseif m == "chunks_percent" then r[m] = r["completed_chunks"] * 100.0 / r["size_chunks"]
			elseif m == "status" then r[m] = status(r)
			elseif m == "eta" then r[m] = eta(r)
			end
		end
	end
	return res
end

function format(method_type, res, methods)
	local formatted = {}
	for _, r in ipairs(res) do
		local item = {}
		for i, v in ipairs(r) do
			item[methods[method_type == "d." and i or i + 1]] = v
		end
		table.insert(formatted, item)
	end
	return formatted
end

function call(method, ...)
	local ok, res = scgi.call(scgi_address, scgi_port, method, ...)
	assert(ok, string.format("XML-RPC call failed on client: %s", tostring(res)))
	return res
end

function multicall(method_type, filter, ...)
	local methods = depends({...})
	local res = call(method_type .. "multicall", filter, unpack(accessor(method_type, methods, "=")))
	local formatted = format(method_type, res, methods)
	return customize(formatted, {...})
end

function batchcall(params, prefix, methods, postfix)
	local p = type(params) == "table" and params or { params }
	local methods_array = {}
	for _, m in ipairs(accessor(prefix, methods, postfix)) do
		table.insert(methods_array, {
			["methodName"] = m,
			["params"] = xmlrpc.newTypedValue(p, "array")
		})
	end
	local res = {}
	for i, r in ipairs(call("system.multicall", xmlrpc.newTypedValue(methods_array, "array"))) do
		res[methods[i]] = r[1]
	end
	return res
end


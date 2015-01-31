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

f = SimpleForm("rtorrent", "Admin")

-- dir = f:field(DummyValue, "dummy", luci.dispatcher.context.authuser)

return f


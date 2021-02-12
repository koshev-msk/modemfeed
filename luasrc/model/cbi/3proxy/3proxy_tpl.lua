local m6, s6, frm
local fs = require "nixio.fs"
local ut = require "luci.util"
local uci = require "luci.model.uci"
local filename = uci.cursor():get_first("3proxy", "3proxy", "config")


m6 = SimpleForm("editing", nil)

m6.submit = translate("Save")
m6.reset = false

s6 = m6:section(SimpleSection, "", translate("Edit config 3proxy"))

frm = s6:option(TextValue, "data")
frm.datatype = "string"
frm.rows = 15


function frm.cfgvalue()
        return fs.readfile(filename) or ""
end


function frm.write(self, section, data)
        return fs.writefile(filename, ut.trim(data:gsub("\r\n", "\n")))
end

return m6

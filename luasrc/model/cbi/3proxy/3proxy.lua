local m

m = Map("3proxy", translate("3proxy"))

s = m:section(TypedSection, "3proxy")
s.anonymous = true

cfg = s:option(Value, "config", translate("Config File"),
	translate("Path to 3proxy config file"))
cfg.rempty = true

function m.on_after_commit(Map)
        luci.sys.call("/etc/init.d/3proxy restart")
end

return m

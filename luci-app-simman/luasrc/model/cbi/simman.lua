--
--

require 'luci.sys'

function split(text, delim)
    -- returns an array of fields based on text and delimiter (one character only)
    local result = {}
    local magic = "().%+-*?[]^$"

    if delim == nil then
        delim = "%s"
    elseif string.find(delim, magic, 1, true) then
        -- escape magic
        delim = "%"..delim
    end

    local pattern = "[^"..delim.."]+"
    for w in string.gmatch(text, pattern) do
        table.insert(result, w)
    end
    return result
end

m = Map("simman", "Simman", translate("SIM manager for modem"))

--- General settings ---
section_gen = m:section(NamedSection, "core", "simman", translate("General settings"))  -- create general section

enabled = section_gen:option(Flag, "enabled", translate("Enabled"), translate("To switch on/off require a reboot"), translate("Enabled"))  -- create enable checkbox
  enabled.rmempty = false

enabled = section_gen:option(Flag, "only_first_sim", translate("Use only high priority SIM"), translate("If you use only one SIM, the remaining SIM will be considered a priority"), translate("Enabled"))
  enabled.rmempty = false

retry_num = section_gen:option(Value, "retry_num",  translate("Number of failed attempts"))
  retry_num.default = 3
  retry_num.datatype = "and(uinteger, min(1))"
  retry_num.rmempty = false
  retry_num.optional = false

check_period = section_gen:option(Value, "check_period",  translate("Period of check, sec"))
  check_period.default = 60
  check_period.datatype = "and(uinteger, min(30))"
  check_period.rmempty = false
  check_period.optional = false

delay = section_gen:option(Value, "delay",  translate("Return to priority SIM, sec"))
  delay.default = 600
  delay.datatype = "and(uinteger, min(60))"
  delay.rmempty = false
  delay.optional = false

csq_level = section_gen:option(Value, "csq_level",  translate("Minimum acceptable signal level, ASU (min: 1, max: 31)"),  translate("0 - not used"))
  csq_level.default = 0
  csq_level.datatype = "and(uinteger, min(0), max(31))"
  csq_level.rmempty = false
  csq_level.optional = false

atdevice = section_gen:option(Value, "atdevice",  translate("AT modem device name"))
  atdevice.default = "/dev/ttyACM3"
  atdevice.datatype = "device"
  atdevice.rmempty = false
  atdevice.optional = false

iface = section_gen:option(Value, "iface",  translate("Ping iface name"))
  iface.default = "internet"
  iface.datatype = "network"
  iface.rmempty = false
  iface.optional = false

testip = section_gen:option(DynamicList, "testip",  translate("IP address of remote servers"))
  testip.datatype = "ipaddr"
  testip.cast = "string"
  testip.rmempty = false
  testip.optional = false

sw_before_modres = section_gen:option(Value, "sw_before_modres",  translate("Switches before modem reset"),  translate("0 - not used"))
  sw_before_modres.default = 0
  sw_before_modres.datatype = "and(uinteger, min(0), max(100))"
  sw_before_modres.rmempty = false
  sw_before_modres.optional = false

sw_before_sysres = section_gen:option(Value, "sw_before_sysres",  translate("Switches before reboot"),  translate("0 - not used"))
  sw_before_sysres.default = 0
  sw_before_sysres.datatype = "and(uinteger, min(0), max(100))"
  sw_before_sysres.rmempty = false
  sw_before_sysres.optional = false


--- SIM info ---
section_info = m:section(NamedSection, "info", "simman", translate("SIM Info"))

  atdevice = section_info:option(Value, "atdevice",  translate("AT modem device name"))
    atdevice.default = "/dev/ttyACM3"
    atdevice.datatype = "device"
    atdevice.rmempty = false
    atdevice.optional = false
  imei = section_info:option(DummyValue, "imei",  translate("Modem IMEI"))
  sim = section_info:option(DummyValue, "sim",  translate("SIM State"))
  ccid = section_info:option(DummyValue, "ccid",  translate("Active SIM CCID"))
  pincode_stat = section_info:option(DummyValue, "pincode_stat",  translate("Pincode Status"))
  sig_lev = section_info:option(DummyValue, "sig_lev",  translate("Signal Strength"))
  reg_stat = section_info:option(DummyValue, "reg_stat",  translate("Registration Status"))
  base_st_id = section_info:option(DummyValue, "base_st_id",  translate("Base Station ID"))
  base_st_bw = section_info:option(DummyValue, "base_st_bw",  translate("Base Station Band"))
  net_type = section_info:option(DummyValue, "net_type",  translate("Cellural Network Type"))
  gprs_reg_stat = section_info:option(DummyValue, "gprs_reg_stat",  translate("GPRS Status"))
  pack_type = section_info:option(DummyValue, "pack_type",  translate("Package Type"))

  refresher = section_info:option( Button, "refresher", translate("Refresh") )  
  refresher.title      = translate("Refresh Info")
  refresher.inputtitle = translate("Refresh")
  refresher.inputstyle = "apply"
  function refresher.write()
     luci.sys.call("/sbin/simman_getinfo")
  end

--- SIM1 settings ---
sim = m:section(TypedSection, "sim0", translate("SIM1 settings"))
  sim.addremove = false
  sim.anonymous = true

priority = sim:option(ListValue, "priority", translate("Priority"))
  priority.default = "1"
  priority:value("0", "low")
  priority:value("1", "high")
  priority.optional = false

GPRS_apn = sim:option(Value, "GPRS_apn", translate("APN"))
  GPRS_apn.default  = ""
  GPRS_apn.rmempty = true
  GPRS_apn.optional = false
  GPRS_apn.cast = "string"

pin = sim:option(Value, "pin", translate("Pincode"))
  pin.default  = ""
  pin.rmempty = true
  pin.optional = false

GPRS_user = sim:option(Value, "GPRS_user", translate("User name"))
  GPRS_user.default  = ""
  GPRS_user.rmempty = true 
  GPRS_user.optional = false

GPRS_pass = sim:option(Value, "GPRS_pass", translate("Password"))
  GPRS_pass.default  = ""
  GPRS_pass.rmempty = true
  GPRS_pass.optional = false

testip = sim:option(DynamicList, "testip",  translate("IP address of remote servers"))
  testip.datatype = "ipaddr"
  testip.cast = "string"

--- SIM2 settings ---
sim = m:section(TypedSection, "sim1", translate("SIM2 settings"))
  sim.addremove = false
  sim.anonymous = true

priority = sim:option(ListValue, "priority", translate("Priority"))
  priority.default = "0"
  priority:value("0", "low")
  priority:value("1", "high")
  priority.optional = false

GPRS_apn = sim:option(Value, "GPRS_apn", translate("APN"))
  GPRS_apn.default  = ""
  GPRS_apn.rmempty = true
  GPRS_apn.optional = false
  GPRS_apn.cast = "string"

pin = sim:option(Value, "pin", translate("Pincode"))
  pin.default  = ""
  pin.rmempty = true
  pin.optional = false

GPRS_user = sim:option(Value, "GPRS_user", translate("User name"))
  GPRS_user.default  = ""
  GPRS_user.rmempty = true
  GPRS_user.optional = false

GPRS_pass = sim:option(Value, "GPRS_pass", translate("Password"))
  GPRS_pass.default  = ""
  GPRS_pass.rmempty = true
  GPRS_pass.optional = false

testip = sim:option(DynamicList, "testip",  translate("IP address of remote servers"))
  testip.datatype = "ipaddr"
  testip.cast = "string"

nbiot = m:section(TypedSection, "nbiot", translate("NB-IoT Modem Info"), translate("if available"))
  nbiot.addremove = false
  nbiot.anonymous = true

nb_imei = nbiot:option(DummyValue, "nb_imei",  translate("NB-IoT modem IMEI"))
  nb_imei.default  = ""

nb_ccid = nbiot:option(DummyValue, "nb_ccid",  translate("NB-IoT SIM CCID"))
  nb_ccid.default  = ""

btn = nbiot:option(Button, "_btn", translate("Refresh"))
  btn.title      = translate("Refresh Info")
  btn.inputtitle = translate("Refresh")
  btn.inputstyle = "apply"
  function btn.write(self, section)
    local test = io.popen("/etc/simman/nbinfo.sh imei")
    local result = test:read("*a")
    test:close()
    nb_imei.value = result

    test = io.popen("/etc/simman/nbinfo.sh ccid")
    local result = test:read("*a")
    test:close()
    nb_ccid.value = result
  end

function m.on_commit(self)
  -- Modified configurations got committed and the CBI is about to restart associated services
end

function m.on_init(self)
  -- The CBI is about to render the Map object
end

return m

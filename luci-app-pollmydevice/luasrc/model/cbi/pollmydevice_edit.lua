local sys = require "luci.sys"
local uci = require "luci.model.uci".cursor()
local d = require "luci.dispatcher"

local section_name

if arg[1] then
	section_name = arg[1]
else
	luci.http.redirect(luci.dispatcher.build_url("admin", "services", "pollmydevice"))
end

local m = Map("pollmydevice", translate("PollMyDevice"), translate("TCP to RS232/RS485 converter"))
	m.redirect=d.build_url("admin/services/pollmydevice/")

local s = m:section(NamedSection, arg[1], "pollmydevice", translate("Utility Settings"))
	s.addremove = false

desc = s:option(Value, "desc", translate("Description"))

devicename = s:option(Value, "devicename", translate("Port"))
	devicename.default = "/dev/com0"
	devicename:value("/dev/com0")
	devicename:value("/dev/com1")

mode = s:option(ListValue, "mode", translate("Mode"))
  mode.default = "disabled"
  mode:value("disabled")
  mode:value("server")
  mode:value("client")
  mode.optional = false

quiet = s:option(Flag, "quiet", translate("Disable log messages"))
  quiet.optional = false
  quiet.default = 0
  quiet:depends("mode","server")
  quiet:depends("mode","client")

baudrate = s:option(ListValue, "baudrate",  translate("BaudRate"))
  baudrate.default = 9600
  baudrate:value(300)
  baudrate:value(600)
  baudrate:value(1200)
  baudrate:value(2400)
  baudrate:value(4800)
  baudrate:value(9600)
  baudrate:value(19200)
  baudrate:value(38400)
  baudrate:value(57600)
  baudrate:value(115200)
  baudrate:value(230400)
  baudrate:value(460800)
  baudrate:value(921600)
  baudrate.optional = false
  baudrate.datatype = "uinteger"
  baudrate:depends("mode","server")
  baudrate:depends("mode","client")

bytesize = s:option(ListValue, "bytesize", translate("ByteSize"))
  bytesize.default = 8
  bytesize:value(5)
  bytesize:value(6)
  bytesize:value(7)
  bytesize:value(8)
  bytesize.optional = false
  bytesize.datatype = "uinteger"
  bytesize:depends("mode","server")
  bytesize:depends("mode","client")

stopbits = s:option(ListValue, "stopbits", translate("StopBits"))
  stopbits.default = 1
  stopbits:value(1)
  stopbits:value(2)
  stopbits.optional = false
  stopbits.datatype = "uinteger"
  stopbits:depends("mode","server")
  stopbits:depends("mode","client")

parity = s:option(ListValue, "parity", translate("Parity"))
  parity.default = "none"
  parity:value("even")
  parity:value("odd")
  parity:value("none")
  parity.optional = false
  parity.datatype = "string"
  parity:depends("mode","server")
  parity:depends("mode","client")

flowcontrol = s:option(ListValue, "flowcontrol", translate("Flow Control"))
  flowcontrol.default = "none"
  flowcontrol:value("XON/XOFF")
  flowcontrol:value("RTS/CTS")
  flowcontrol:value("none")
  flowcontrol.optional = false
  flowcontrol.datatype = "string"
  flowcontrol:depends("mode","server")
  flowcontrol:depends("mode","client")

server_port = s:option(Value, "server_port",  translate("Server Port"))
  server_port.datatype = "and(uinteger, min(1025), max(65535))"
  --server_port.rmempty = false
  server_port:depends("mode","server")

connection_mode = s:option(ListValue, "connection_mode", translate("Connection Mode"))
  connection_mode.default = 0
  connection_mode:value(0,translate("Alternating"))
  connection_mode:value(1,translate("Simultaneous"))
  connection_mode:depends("mode","server")

holdconntime = s:option(Value, "holdconntime",  translate("Connection Hold Time (sec)"))
  holdconntime.default = 60
  holdconntime.datatype = "and(uinteger, min(0), max(100000))"
  --holdconntime.rmempty = false
  holdconntime:depends("connection_mode",0)

pack_size = s:option(Value, "pack_size",  translate("Minimum packet size [0-255] (byte)"), translate("Minimum data packet size to send. 0 - not used"))
  pack_size.default = 0
  pack_size.datatype = "and(uinteger, min(0), max(255))"
  pack_size:depends("mode","server")
  pack_size:depends("mode","client")

pack_timeout = s:option(Value, "pack_timeout",  translate("Packet timeout [0-255] (x100ms)"), translate("Time of data accumulation before sending. 0 - not used"))
  pack_timeout.default = 0
  pack_timeout.datatype = "and(uinteger, min(0), max(255))"
  for i=1,255 do pack_timeout:depends("pack_size",i) end

client_host = s:option(Value, "client_host",  translate("Server Host or IP Address"))
  client_host.default = "hub.m2m24.ru"
  client_host.datatype = "string"
  client_host:depends("mode","client")

client_port = s:option(Value, "client_port",  translate("Server Port"))
  client_port.default = 6008
  client_port.datatype = "and(uinteger, min(1025), max(65535))"
  --client_port.rmempty = false
  client_port:depends("mode","client")

client_timeout = s:option(Value, "client_timeout",  translate("Client Reconnection Timeout (sec)"))
  client_timeout.default = 60
  client_timeout.datatype = "and(uinteger, min(0), max(100000))"
  --client_timeout.rmempty = false
  client_timeout:depends("mode","client")

client_auth = s:option(ListValue, "client_auth", translate("Client Authentification"), translate("Use Teleofis Authentification"))
  client_auth.widget="radio"
  client_auth.default = 0
  client_auth:value(0,"Disable")
  client_auth:value(1,"Enable")
  --client_auth.rmempty = false
  client_auth:depends("mode","client")

teleofisid = s:option(DummyValue, "teleofisid",  translate("Teleofis ID"))
  --client_auth.rmempty = false
  teleofisid:depends("mode","client")

modbus_gateway = s:option(ListValue, "modbus_gateway", translate("Modbus TCP/IP"))  -- create checkbox
  modbus_gateway.default = 0
  modbus_gateway:value(0,"Disabled")
  modbus_gateway:value(1,"RTU")
  modbus_gateway:value(2,"ASCII")
  modbus_gateway:depends("mode","server")
  modbus_gateway:depends("client_auth",0)

coff = s:option(Button, "coff", translate("Disable console port"), translate("Save the changes. The router will reboot"))  
  coff.title      = translate("Disable console port")
  coff.inputtitle = translate("Disable")
  coff.inputstyle = "apply"
  coff:depends("devicename","/dev/com0")
  function coff.write()
     luci.sys.call("/etc/pollmydevice/console disable")
  end

con = s:option(Button, "con", translate("Enable console port"), translate("Save the changes. The router will reboot"))  
  con.title      = translate("Enable console port")
  con.inputtitle = translate("Enable")
  con.inputstyle = "apply"
  con:depends("devicename","/dev/com0")
  function con.write()
     luci.sys.call("/etc/pollmydevice/console enable")
  end

return m
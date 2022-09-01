common_path = '/usr/share/gpoint/lib/?.lua;'
package.path = common_path .. package.path

local nmea = require("nmea")
local serial = require("serial")
local nixio   = require("nixio.fs")

local que = {}

local QUECTEL_BEGIN_GPS  = "AT+QGPS=1"
local QUECTEL_END_GPS    = "AT+QGPSEND"

-- automatic activation of the NNM port for data transmission
function que.start(port)
	local p = tonumber(string.sub(port, #port)) + 1
	p = string.gsub(port, '%d', tostring(p))
	local error, resp = true, {warning = {
							   app = {true, "Port is unavailable. Check the modem connections!"},
				               locator = {}, 
				               server = {}
				               }
			                }
	local fport = nixio.glob("/dev/tty[A-Z][A-Z]*")
	for name in fport do
		if string.find(name, p) then
			error, resp = serial.write(p, QUECTEL_BEGIN_GPS)
		end
	end
	return error, resp
end
-- quactel stop send data to NMEA port
function que.stop(port)
	error, resp = serial.write(port, QUECTEL_END_GPS)
	return error, resp
end
-- get data for the application
function que.getGNSSdata(port)
	return nmea.getAllData(port)
end
	
return que

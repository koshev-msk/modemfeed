common_path = '/usr/share/gpoint/lib/?.lua;'
package.path = common_path .. package.path


local nmea = require("nmea")
local serial = require("serial")
local nixio   = require("nixio.fs")

local sierra = {}

local SIERRA_BEGIN_GPS  = "$GPS_START"
local SIERRA_END_GPS    = "$GPS_STOP"

-- automatic activation of the NMEA port for data transmission
function sierra.start(port)
	local error, resp = true, {warning = {
							   app = {true, "Port is unavailable. Check the modem connections!"},
				               locator = {}, 
				               server = {}
				               }
			                }
	local fport = nixio.glob("/dev/tty[A-Z][A-Z]*")
	for name in fport do
		if string.find(name, port) then
			error, resp = serial.write(port, SIERRA_BEGIN_GPS)
		end
	end
	return error, resp
end
-- stop send data to NMEA port
function sierra.stop(port)
	error, resp = serial.write(port, SIERRA_END_GPS)
	return error, resp
end
-- get GNSS data for application
function sierra.getGNSSdata(port)
	return nmea.getAllData(port)
end
	
return sierra
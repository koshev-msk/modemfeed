common_path = '/usr/share/gpoint/lib/?.lua;'
package.path = common_path .. package.path


local nmea = require("nmea")
local serial = require("serial")
local nixio   = require("nixio.fs")

local simcom = {}

local SIMCOM_BEGIN_GPS  = "AT+CGPS=1,1"
local SIMCOM_END_GPS    = "AT+CGPS=0,1"

-- automatic activation of the NMEA port for data transmission
function simcom.start(port)
	local p = tonumber(string.sub(port, #port)) + 1
	p = string.gsub(port, '%d', tostring(p))
	local error, resp = true, {
		warning = {
			app = {true, "Port is unavailable. Check the modem connections!"},
			locator = {}, 
			server = {}
		}
	}
	-- SIM7600 series default NMEA /dev/ttyUSB1
	local fport = nixio.glob("/dev/tty[A-Z][A-Z]*")
	for name in fport do
		if string.find(name, p) then
			error, resp = serial.write(p, SIMCOM_BEGIN_GPS)
		end
	end
	return error, resp
end
-- stop send data to NMEA port
function simcom.stop(port)
	error, resp = serial.write(port, SIMCOM_END_GPS)
	return error, resp
end
-- get GNSS data for application
function simcom.getGNSSdata(port)
	return nmea.getAllData(port)
end

return simcom	

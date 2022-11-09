common_path = '/usr/share/gpoint/lib/?.lua;'
package.path = common_path .. package.path


local nmea = require("nmea")

local ublox = {}

-- Wrapper over the interface, the module does not need an implementation
function ublox.start(port)
	return false, {warning = {app = {false, "GOOD!"}, locator = {}, server = {}}}
end

function ublox.stop(port)
	return {false, "OK"}
end
-- get GNSS data for application
function ublox.getGNSSdata(port)
	return nmea.getAllData(port)
end

return ublox
-------------------------------------------------------------
-- Traccar Client use this protocol to report GPS data to the server side. 
-- OsmAnd Live Tracking web address format:
-- http://demo.traccar.org:5055/?id=123456&lat={0}&lon={1}&timestamp={2}&hdop={3}&altitude={4}&speed={5}
-------------------------------------------------------------
-- Copyright 2021-2022 Vladislav Kadulin <spanky@yandex.ru>
-- Licensed to the GNU General Public License v3.0


local http = require("socket.http")

local trackcar = {}

local function UnixTime(time, date)
	local datetime = { year,month,day,hour,min,sec }
	datetime.hour, datetime.min, datetime.sec   = string.match(time, "(%d%d)(%d%d)(%d%d)")
	datetime.day, datetime.month, datetime.year = string.match(date,"(%d%d)(%d%d)(%d%d)")
	datetime.year = "20" .. datetime.year
	return os.time(datetime)
end

local function OsmAnd(GnssData, serverConfig)
    local unix = GnssData.warning.rmc[1] and os.time() or UnixTime(GnssData.rmc.utc, GnssData.rmc.date)
	return string.format("http://%s:%s/?id=%s&lat=%s&lon=%s&timestamp=%s&hdop=%s&altitude=%s&speed=%s&satellites=%s", 
			serverConfig.address, serverConfig.port, serverConfig.login,
			GnssData.gp.latitude  or '-', GnssData.gp.longitude or '-',
			unix                  or '-', GnssData.gp.hdop      or '-',		
			GnssData.gp.altitude  or '-', GnssData.gp.spkm      or '-',
			GnssData.gp.nsat      or '-')
end

-- Send data to server side
function trackcar.sendData(GnssData, serverConfig)
	local po = OsmAnd(GnssData, serverConfig)
	http.TIMEOUT = 0.5
	local err = http.request{ method = "POST", url = po}
	return err
end

return trackcar
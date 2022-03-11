<h2 align="center">
 <img src="https://github.com/Kodo-kakaku/luci-app-gpoint/blob/main/Images/logo.png" alt="Gpoint" height="200" width="370">
  <br>Global Navigation Satellite System for OpenWrt LuCi<br>
</h2>
<p align="center">Gpoint was created in order to use full set of functions of mobile modules installed in OpenWRT router.
Manufacturers of GSM/3G/LTE modems often lay down GNSS function, so why not use it?
It doesn't matter if you use a router in transport or it is installed in your terminal, you can always find out its location!</p>

## Features
- Support: GPS, GLONASS (works with "NMEA 0183" standard protocol)
- GeoHash (reduces drift of GPS\GLONASS coordinate readings in parking)
- Yandex Locator [API](https://yandex.ru/dev/locator/) (Determines location by nearest Wi-Fi access points - without using satellite navigation systems)
- Server side (sends GNSS data to a remote server)
- Support [OpenLayers](https://openlayers.org/) maps in UI, and much more!

## Supported devices
- Quectel EC25/EP06/EM12

## Supported GNSS protocols
- [OsmAnd](https://www.traccar.org/osmand/)
- [Wialon IPS](https://gurtam.com/ru/gps-hardware/soft/wialon-ips)

## Install
- Upload ipk file to tmp folder
- cd /tmp
- opkg update
- opkg install luci-app-gpoint_1.2.3_all.ipk

## Uninstall
- opkg remove luci-app-gpoint

## License  
Gpoint like OpenWRT is released under the GPL v3.0 License - see detailed [LICENSE](https://github.com/Kodo-kakaku/luci-app-gpoint/blob/main/LICENSE).

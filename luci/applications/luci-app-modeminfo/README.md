# luci-app-modeminfo
3G/LTE dongle information for OpenWrt LuCi


luci-app-modeminfo is fork from https://github.com/IceG2020/luci-app-3ginfo

Supported devices:

 - Quectel EC200T/EC21/EC25/EP06/EM12

 - SimCom SIM7600E-H

 - Huawei E3372 (LTE)/ME909

 - Sierra Wireless EM7455

 - HP LT4220

 - Dell DW5821e
 
 - MikroTik R11e-LTE/R11e-LTE6 (temporary dropped)

 - Fibocom NL668/NL678/L850/L860

<details>
<summary>Package contents:</summary>

|Package |Description |
|:-------|:-----------|
|luci-app-modeminfo |LuCI web interface |
|modeminfo |common files |
|modeminfo-qmi |Qualcomm MSM Interface support |
|modeminfo-serial-quectel |Quectel modems support |
|modeminfo-serial-telit |Telit LN940 (HP LT4220) modem support |
|modeminfo-serial-huawei |Huawei ME909/E3372(stick mode, LTE only) modems support|
|modeminfo-serial-sierra |Sierra EM7455 modem support |
|modeminfo-serial-simcom |SimCOM modems support |
|modeminfo-serial-dell |Dell DW5821e modem support |
|modeminfo-serial-fibocom |Fibocom LN668/NL678 modems support |
|modeminfo-serial-xmm |Fibocom L850/L860 modems support |
</details>

<details>
   <summary>Screenshots</summary>
   
   
* Overview page. Short network info.

   ![](https://raw.githubusercontent.com/koshev-msk/modemfeed/master/luci/applications/luci-app-modeminfo/screenshots/modeminfo-overview.png)
   
* Modeminfo index page. Verbose network info.

   ![](https://raw.githubusercontent.com/koshev-msk/modemfeed/master/luci/applications/luci-app-modeminfo/screenshots/modeminfo-network.png)
   
* Modeminfo hardware page.

   ![](https://raw.githubusercontent.com/koshev-msk/modemfeed/master/luci/applications/luci-app-modeminfo/screenshots/modeminfo-hardware.png)

* Modeminfo setup page.

   ![](https://raw.githubusercontent.com/koshev-msk/modemfeed/master/luci/applications/luci-app-modeminfo/screenshots/modeminfo-setup.png)

</details>


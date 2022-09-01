local nixio = require "nixio"

module("luci.controller.modem.cellled", package.seeall)

local utl = require "luci.util"

function index()
	entry({"admin", "modem"},  firstchild(), "Modem", 45).acl_depends={"unauthenticated"}
	entry({"admin", "modem", "cellled"}, cbi("modem/cellled"), translate("CellLED"), 63).acl_depends={"unauthenticated"}
end


module("luci.controller.simman", package.seeall)

function index()
	if not nixio.fs.access("/etc/config/simman") then
		return
	end

	local page

	page = entry({"admin", "services", "simman"}, cbi("simman"), _(translate("Simman")))
	page.dependent = true
end

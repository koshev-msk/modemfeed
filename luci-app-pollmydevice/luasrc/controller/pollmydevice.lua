module("luci.controller.pollmydevice", package.seeall)

function index()
	entry( {"admin", "services", "pollmydevice"},  arcombine(cbi("pollmydevice_add"), cbi("pollmydevice_edit")), _("PollMyDevice"), 5).leaf=true
end

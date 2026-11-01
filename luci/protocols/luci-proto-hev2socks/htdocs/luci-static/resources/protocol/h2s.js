'use strict';
'require form';
'require network';

return network.registerProtocol('h2s', {
	getI18n: function() {
		return _('Hev SOCKS5 Tunnel');
	},

	getIfname: function() {
		return this._ubus('l3_device') || this.sid;
	},

	getOpkgPackage: function() {
		return 'hev-socks5-tunnel';
	},

	isFloating: function() {
		return true;
	},

	isVirtual: function() {
		return true;
	},

	getDevices: function() {
		return null;
	},

	containsDevice: function(ifname) {
		return (network.getIfnameOf(ifname) == this.getIfname());
	},

	renderFormOptions: function(s) {
		var dev = this.getL3Device() || this.getDevice(), o;

		o = s.taboption('general', form.Value, 'server', _('SOCKS5 server'),
			_('IP address or hostname of the SOCKS5 server'));
		o.datatype = 'or(host,ipaddr)';
		o.rmempty = false;

		o = s.taboption('general', form.Value, 'port', _('SOCKS5 port'));
		o.datatype = 'port';
		o.placeholder = '1080';
		o.rmempty = false;

		o = s.taboption('general', form.Value, 'username', _('Username'),
			_('Both username and password must be set, or both left empty'));
		o.rmempty = true;

		o = s.taboption('general', form.Value, 'password', _('Password'));
		o.password = true;
		o.rmempty = true;

		o = s.taboption('general', form.ListValue, 'udp', _('UDP relay mode'));
		o.value('udp', _('UDP (native SOCKS5 UDP ASSOCIATE)'));
		o.value('tcp', _('UDP over TCP'));
		o.default = 'udp';

		o = s.taboption('general', form.DynamicList, 'ipaddr', _('Tunnel addresses'),
			_('IPv4 and/or IPv6, CIDR notation. Without a mask /32 or /128 is used'));
		o.datatype = 'ipmask';
		o.rmempty = true;

		o = s.taboption('general', form.DynamicList, 'gateway', _('Gateway'),
			_('Adds a default route through this address'));
		o.datatype = 'ipaddr("nomask")';
		o.rmempty = true;

		o = s.taboption('advanced', form.Value, 'mtu', _('Override MTU'),
			_('Tunnel device maximum transmission unit'));
		o.placeholder = '8500';
		o.datatype = 'range(576,9200)';

		o = s.taboption('advanced', form.Value, 'mark', _('Firewall mark'),
			_('Mark for packets of the SOCKS5 connection itself'));
		o.datatype = 'uinteger';
		o.rmempty = true;

		o = s.taboption('advanced', form.Flag, 'tcp_fast_open', _('TCP Fast Open'));
		o.rmempty = true;

		o = s.taboption('advanced', form.ListValue, 'log_level', _('Log level'));
		o.value('debug', _('Debug'));
		o.value('info', _('Info'));
		o.value('warn', _('Warning'));
		o.value('error', _('Error'));
		o.default = 'warn';
	}
});

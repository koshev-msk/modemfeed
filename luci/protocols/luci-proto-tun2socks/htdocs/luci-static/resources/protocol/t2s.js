'use strict';
'require rpc';
'require form';
'require network';
'require validation';

network.registerPatternVirtual(/^t2s-.+$/);

return network.registerProtocol('t2s', {
	getI18n: function() {
		return _('tun2socks');
	},

	getIfname: function() {
		return this._ubus('l3_device') || 't2s-%s'.format(this.sid);
	},

	getOpkgPackage: function() {
		return 'tun2socks';
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

		o = s.taboption('general', form.Value, 'ipaddr', _('IPv4 Address'));
		o.datatype = 'ip4addr("nomask")';
		o.rmempty = false;

		o = s.taboption('general', form.Value, 'netmask', _('IPv4 Netmask'));
		o.value('255.255.255.0', '255.255.255.0');
		o.value('255.255.0.0', '255.255.0.0');
		
		o.rmempty = false;

		o = s.taboption('general', form.Value, 'gateway', _('IPv4 Gateway'));
		o.datatype = 'ip4addr("nomask")';
		o.rmempty = false;

		o = s.taboption('general', form.ListValue, 'proxy', _('Proxy Type'));
		o.value('socks4', 'SOCKS4');
		o.value('socks5', 'SOCKS5');
		o.value('http', 'HTTP');
		o.value('ss', 'Shadowsocks');
		o.value('relay', _('Relay'));
		o.value('direct', _('Direct'));
		o.value('reject', _('Reject'));
		o.rmempty = true;

		o = s.taboption('general', form.Value, 'host', _('Proxy Address'));
		o.datatype = 'or(hostname,ip4addr("nomask"))';
		o.rmempty = false;

		o = s.taboption('general', form.Value, 'port', _('Proxy PORT'));
		o.datatype = 'range(1, 65535)';
		o.rmempty = false;

		o = s.taboption('general', form.Flag, 'advanced', _('Autentification'), _('Authentification and encryption.'));
		o.rmempty = true;

		o = s.taboption('general', form.Value, 'username', _('Proxy USER'));
		o.depends('advanced', '1');
		
		o = s.taboption('general', form.Value, 'password', _('Proxy Password'));
		o.password = true;
		o.depends('advanced', '1');

		o = s.taboption('general', form.Value, 'encrypt', _('Encryption'), _('Example: aes-256-gcm'));
		o.depends('advanced', '1');

		o = s.taboption('advanced', form.Value, 'mtu', _('Set MTU'), _('Set device maximum transmission unit'));
		o.placeholder = dev ? (dev.getMTU() || '1500') : '1500';
		o.datatype    = 'max(9200)';
		
		o = s.taboption('advanced', form.ListValue, 'loglevel', _('Logging level'));
		o.value('debug', _('Debug'));
		o.value('info', _('Info'));
		o.value('warning', _('Warning'));
		o.value('error', _('Error'));
		o.value('silent', _('Silent'));
		o.default = 'error';
		
		o = s.taboption('advanced', form.Value, 'opts', _('Advaced options'), _('Command line arguments to tun2socks app'));
		o.rmempty = true;

		o = s.taboption('advanced', form.Flag, 'defaultroute',
			_('Use default gateway'),
			_('If unchecked, no default route is configured'));
		o.default = o.enabled;

		o = s.taboption('advanced', form.Value, 'metric',
			_('Use gateway metric'));
			o.placeholder = '0';
		o.datatype = 'uinteger';
		o.depends('defaultroute', '1');

		o = s.taboption('advanced', form.Flag, 'peerdns',
			_('Use DNS servers advertised by peer'),
			_('If unchecked, the advertised DNS server addresses are ignored'));
		o.default = o.enabled;

	}
});


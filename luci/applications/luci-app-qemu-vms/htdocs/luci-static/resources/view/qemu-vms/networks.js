'use strict';
'require form';
'require view';
'require uci';
'require rpc';

var callListNetDrivers = rpc.declare({
	object: 'luci.qemu-vms',
	method: 'list_net_drivers',
	params: []
});

function randomMac() {
	var b1 = Math.floor(Math.random() * 256).toString(16).padStart(2, '0');
	var b2 = Math.floor(Math.random() * 256).toString(16).padStart(2, '0');
	return '52:54:00:31:%s:%s'.format(b1, b2);
}

return view.extend({
	load: function() {
		return Promise.all([
			callListNetDrivers(),
			uci.load('qemu-vms'),
			uci.load('network')
		]);
	},

	render: function(data) {
		var drivers = data[0].drivers || [];
		var m, s;

		m = new form.Map('qemu-vms', _('Virtual network segments'),
			_('Define reusable tap interfaces that can be attached to one or more VMs.') +
			'<br />' + _('Interfaces are brought up by /etc/qemu-ifup at VM start; if "bridge" is set,') +
			' ' + _('the interface is also enslaved into that bridge, otherwise it is left standalone.'));

		s = m.section(form.GridSection, 'network', _('Networks'));
		s.addremove = true;
		s.anonymous = false;
		s.nodescriptions = true;

		var mac = s.option(form.Value, 'mac', _('MAC address'));
		mac.validate = function(section_id, value) {
			if (!value)
				return true;
			if (!/^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$/.test(value))
				return _('Invalid MAC address format (expected xx:xx:xx:xx:xx:xx)');
			return true;
		};

		mac.rmempty = false;
		mac.renderWidget = function(section_id, option_index, cfgvalue) {
			this.keylist = [randomMac()];
			this.vallist = [_('Automatically assigned')];
			return form.Value.prototype.renderWidget.apply(this, arguments);
		};

		var ifname = s.option(form.Value, 'ifname', _('tap ifname'));
		ifname.placeholder = 'tap0';
		ifname.rmempty = false;
		ifname.validate = function(section_id, value) {
			if (!value)
				return _('Required');
			return true;
		};

		// В render:
		var bridgeList = [];
		uci.sections('network', 'device').forEach(function(s) {
			if (s.type === 'bridge') {
				bridgeList.push(s.name);
			}
		});

		var bridge = s.option(form.Value, 'bridge', _('Bridge (optional)'));
		bridge.value('', _('not assign'));
		bridgeList.forEach(function(name) {
			bridge.value(name, name);
		});
		bridge.rmempty = true;
		//bridge.nocreate = false;   

		var driver = s.option(form.ListValue, 'driver', _('Network driver'));

		if (drivers.length) {
			drivers.forEach(function(d) {
				driver.value(d, d);
			});
		} else {
			driver.value('', _('No drivers found'));
		}
		driver.default = 'virtio-net-pci';
		driver.rmempty = true;
		driver.description = _('Choose the emulated network card model. Default driver: <code>virtio-net-pci</code>');

		return m.render();
	}
});

'use strict';
'require form';
'require view';
'require uci';

function randomMac() {
	// OUI 52:54:00:31 is our virtual-cluster prefix (matches manually
	// assigned MACs used elsewhere in this config).
	var b1 = Math.floor(Math.random() * 256).toString(16).padStart(2, '0');
	var b2 = Math.floor(Math.random() * 256).toString(16).padStart(2, '0');
	return '52:54:00:31:%s:%s'.format(b1, b2);
}

return view.extend({
	render: function() {
		var m, s;

		m = new form.Map('qemu-vms', _('Virtual network segments'),
			_('Define reusable tap interfaces that can be attached to one or more VMs. ' +
				'Interfaces are brought up by /etc/qemu-ifup at VM start; if "bridge" is set, ' +
				'the interface is also enslaved into that bridge, otherwise it is left standalone.'));

		s = m.section(form.GridSection, 'network', _('Networks'));
		s.addremove = true;
		s.anonymous = false;
		s.nodescriptions = true;

		var mac = s.option(form.Value, 'mac', _('MAC address'));
		//mac.placeholder = _('leave empty to auto-generate 52:54:00:31:xx:xx');
		mac.validate = function(section_id, value) {
			if (!value)
				return true; // filled in by write() below if left empty
			if (!/^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$/.test(value))
				return _('Invalid MAC address format (expected xx:xx:xx:xx:xx:xx)');
			return true;
		};
		mac.write = function(section_id, value) {
			if (!value)
				value = randomMac();
			return form.Value.prototype.write.call(this, section_id, value);
		};
		mac.rmempty = false;
		mac.value(randomMac(), _('Automatically assigned'));
		//mac.editable = true;

		var ifname = s.option(form.Value, 'ifname', _('tap ifname'));
		ifname.placeholder = 'tap0';
		ifname.rmempty = false;
		ifname.validate = function(section_id, value) {
			if (!value)
				return _('Required');
			return true;
		};

		var bridge = s.option(form.Value, 'bridge', _('Bridge (optional)'));
		bridge.placeholder = _('leave empty to keep the interface standalone');

		return m.render();
	}
});

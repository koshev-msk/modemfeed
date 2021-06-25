'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require tools.widgets as widgets';

return view.extend({
	handleEnableService: rpc.declare({
		object: 'luci',
		method: 'setInitAction',
		params: [ 'pingcontrol', 'enable' ],
		expect: { result: false }
	}),

	render: function() {
		var m, s, o;
		
		m = new form.Map('pingcontrol', _('PingControl'));
		m.description = _('Server availability check');

		s = m.section(form.GridSection, 'pingcontrol', _('Settings'));
		s.addremove = true;
		s.nodescriptions = true;

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.rmempty = false;
		o.write = L.bind(function(section, value) {
			if (value == '1') {
				this.handleEnableService();
			}
			return uci.set('pingcontrol', section, 'enabled', value);
		}, this);

		o = s.option(widgets.NetworkSelect, 'iface', _('Ping interface'));
		o.rmempty = false;
		o.textvalue = function(section_id) {
			return uci.get('pingcontrol', section_id, 'iface');
		}

		o = s.option(form.DynamicList, 'testip', _('IP address of remote servers'));
		o.datatype = 'ipaddr';

		o = s.option(form.Value, 'check_period', _('Period of check, sec'));
		o.rmempty = false;
		o.modalonly = true;
		o.datatype = 'and(uinteger,min(20))';
		o.default = '60';

		o = s.option(form.Value, 'sw_before_modres', _('Failed attempts before iface up/down'), _('0 - not used'));
		o.rmempty = false;
		o.datatype = 'and(uinteger,min(0),max(100))';
		o.default = '3';

		o = s.option(form.Value, 'sw_before_sysres', _('Failed attempts before reboot'), _('0 - not used'));
		o.rmempty = false;
		o.datatype = 'and(uinteger,min(0),max(100))';
		o.default = '0';

		return m.render();
	}
});

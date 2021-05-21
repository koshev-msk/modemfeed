'use strict';
'require fs';
'require rpc';
'require uci';
'require view';
'require form';
'require tools.widgets as widgets';

return view.extend({
	handleEnableService: rpc.declare({
		object: 'luci',
		method: 'setInitAction',
		params: [ 'simman2', 'enable' ],
		expect: { result: false }
	}),

	render: function() {
		var m, s, o;
		
		m = new form.Map('simman2', _('Simman2'));
		m.description = _('SIM manager for modem');

		s = m.section(form.GridSection, 'simman2', _('Settings'));
		s.tab('general', _('General Settings'));
		s.tab('sim_cards', _('SIM Settings'));
		s.tab('info', _('Information'));
		s.addremove = true;
		s.nodescriptions = true;

		o = s.taboption('general', form.Flag, 'enabled', _('Enabled'));
		o.rmempty = false;
		o.write = L.bind(function(section, value) {
			if (value == '1') {
				this.handleEnableService();
			}
			return uci.set('simman2', section, 'enabled', value);
		}, this);

		o = s.taboption('general', widgets.NetworkSelect, 'interface', _('Interface name'));
		o.rmempty = false;

		o = s.taboption('general', form.Value, 'atdevice', _('AT modem device name'));
		o.rmempty = false;

		o = s.taboption('general', form.Flag, 'only_first_sim', _('Use only high priority SIM'), _('If you use only one SIM, the remaining SIM will be considered a priority'));
		o.rmempty = false;
		o.modalonly = true;

		o = s.taboption('general', form.DynamicList, 'testip', _('IP address of remote servers'));
		o.rmempty = false;
		o.datatype = 'ipaddr';

		o = s.taboption('general', form.Value, 'csq_level', _('Minimum acceptable signal level, ASU (min: 1, max: 31)'), _('0 - not used'));
		o.rmempty = false;
		o.datatype = 'and(uinteger, min(0), max(31))'
		o.default = '0';
		o.modalonly = true;

		o = s.taboption('general', form.Value, 'retry_num', _('Number of failed attempts'));
		o.rmempty = false;
		o.datatype = 'and(uinteger,min(1))';
		o.default = '3';
		o.modalonly = true;

		o = s.taboption('general', form.Value, 'check_period', _('Period of check, sec'));
		o.rmempty = false;
		o.datatype = 'and(uinteger,min(30))';
		o.default = '60';
		o.modalonly = true;

		o = s.taboption('general', form.Value, 'sw_before_modres', _('Switches before modem reset'), _('0 - not used'));
		o.rmempty = false;
		o.datatype = 'and(uinteger,min(0),max(100))';
		o.default = '0';

		o = s.taboption('general', form.Value, 'sw_before_sysres', _('Switches before reboot'), _('0 - not used'));
		o.rmempty = false;
		o.datatype = 'and(uinteger,min(0),max(100))';
		o.default = '0';

		o = s.taboption('general', form.Value, 'delay', _('Return to priority SIM, sec'));
		o.rmempty = false;
		o.datatype = 'and(uinteger,min(60))';
		o.default = '600';
		o.modalonly = true;

		o = s.taboption('sim_cards', form.DummyValue, 'sim0', _('SIM1 configuration'));
		o.default = '';
		o.modalonly = true;

		o = s.taboption('sim_cards', form.ListValue, 'sim0_priority', _('Priority'));
		o.default = '1';
		o.value('0','low');
		o.value('1','high');
		o.rmempty = false;
		o.modalonly = true;

		o = s.taboption('sim_cards', form.Value, 'sim0_apn', _('APN'));
		o.modalonly = true;

		o = s.taboption('sim_cards', form.Value, 'sim0_pincode', _('Pincode'));
		o.modalonly = true;

		o = s.taboption('sim_cards', form.Value, 'sim0_username', _('User name'));
		o.modalonly = true;

		o = s.taboption('sim_cards', form.Value, 'sim0_password', _('Password'));
		o.modalonly = true;
		o.password = true;

		o = s.taboption('sim_cards', form.DynamicList, 'sim0_testip', _('IP address of remote servers'));
		o.datatype = 'ipaddr';
		o.modalonly = true;

		o = s.taboption('sim_cards', form.DummyValue, 'sim1', _('SIM2 configuration'));
		o.default = '';
		o.modalonly = true;

		o = s.taboption('sim_cards', form.ListValue, 'sim1_priority', _('Priority'));
		o.default = '1';
		o.value('0','low');
		o.value('1','high');
		o.rmempty = false;
		o.modalonly = true;

		o = s.taboption('sim_cards', form.Value, 'sim1_apn', _('APN'));
		o.modalonly = true;

		o = s.taboption('sim_cards', form.Value, 'sim1_pincode', _('Pincode'));
		o.modalonly = true;

		o = s.taboption('sim_cards', form.Value, 'sim1_username', _('User name'));
		o.modalonly = true;

		o = s.taboption('sim_cards', form.Value, 'sim1_password', _('Password'));
		o.modalonly = true;
		o.password = true;

		o = s.taboption('sim_cards', form.DynamicList, 'sim1_testip', _('IP address of remote servers'));
		o.datatype = 'ipaddr';
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'imei', _('Modem IMEI'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'sim', _('SIM State'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'ccid', _('Active SIM CCID'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'pincode_stat', _('Pincode Status'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'sig_lev', _('Signal Strength'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'reg_stat', _('Registration Status'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'base_st_id', _('Base Station ID'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'base_st_bw', _('Base Station Band'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'net_type', _('Cellural Network Type'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'gprs_reg_stat', _('GPRS Status'));
		o.modalonly = true;

		o = s.taboption('info', form.DummyValue, 'pack_type', _('Package Type'));
		o.modalonly = true;

		return m.render();
	}
});

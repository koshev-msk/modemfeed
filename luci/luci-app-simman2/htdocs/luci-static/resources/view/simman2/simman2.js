'use strict';
'require fs';
'require rpc';
'require uci';
'require ui';
'require view';
'require form';
'require tools.widgets as widgets';

var modemRequestAll = rpc.declare({
	object: 'simman2',
	method: 'statusall',
	params: [ 'config' ],
	expect: { '': {} }
});

var DummyValueExt = form.DummyValue.extend({
	renderWidget: function(section_id, option_index, cfgvalue) {
		return E([], [
			E('div', {
				'class': 'cbi-value-field',
				'id': this.cbid(section_id),
				'style': 'color:#c73d3d;margin-left:0'
			})
		]);
	}
});

return view.extend({
	handleEnableService: rpc.declare({
		object: 'luci',
		method: 'setInitAction',
		params:  [ 'simman2', 'enable' ],
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

		o = s.taboption('general', widgets.NetworkSelect, 'iface', _('Interface name'));
		o.rmempty = false;
		o.textvalue = function(section_id) {
			return uci.get('simman2', section_id, 'iface');
		}

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

		o = s.taboption('info', DummyValueExt, 'modem', _('Modem'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'firmware', _('Firmware version'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'imei', _('Modem IMEI'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'ccid', _('Active SIM CCID'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'imsi', _('Active SIM IMSI'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'sim', _('SIM State'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'pincode_stat', _('Pincode Status'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'sig_lev', _('Signal Strength'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'reg_stat', _('Registration Status'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'net_type', _('Cellural Network Type'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'gprs_reg_stat', _('GPRS Status'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'pack_type', _('Package Type'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'base_st_id', _('Base Station ID'));
		o.modalonly = true;

		o = s.taboption('info', DummyValueExt, 'base_st_bw', _('Base Station Band'));
		o.modalonly = true;		

		o = s.taboption('info', form.Button, 'refresh',_(' '));
		o.modalonly = true;
		o.inputstyle = 'action important';
		o.inputtitle = _('Refresh');
		o.onclick = L.bind(function(ev, section_id) {
			return modemRequestAll(section_id).then(function(t) {
				document.getElementById('cbid.simman2.%s.modem'.format(section_id)).textContent = t.modem || 'n/a';
				document.getElementById('cbid.simman2.%s.firmware'.format(section_id)).textContent = t.firmware || 'n/a';
				document.getElementById('cbid.simman2.%s.imei'.format(section_id)).textContent = t.imei || 'n/a';
				document.getElementById('cbid.simman2.%s.ccid'.format(section_id)).textContent = t.ccid || 'n/a';
				document.getElementById('cbid.simman2.%s.imsi'.format(section_id)).textContent = t.imsi || 'n/a';
				document.getElementById('cbid.simman2.%s.sim'.format(section_id)).textContent = t.sim_state || 'n/a';
				document.getElementById('cbid.simman2.%s.pincode_stat'.format(section_id)).textContent = t.pin_state || 'n/a';
				document.getElementById('cbid.simman2.%s.sig_lev'.format(section_id)).textContent = t.csq || 'n/a';
				document.getElementById('cbid.simman2.%s.reg_stat'.format(section_id)).textContent = t.net_reg || 'n/a';				
				document.getElementById('cbid.simman2.%s.net_type'.format(section_id)).textContent = t.net_type || 'n/a';
				document.getElementById('cbid.simman2.%s.gprs_reg_stat'.format(section_id)).textContent = t.data_reg || 'n/a';
				document.getElementById('cbid.simman2.%s.pack_type'.format(section_id)).textContent = t.data_type || 'n/a';
				document.getElementById('cbid.simman2.%s.base_st_id'.format(section_id)).textContent = t.bs_id || 'n/a';
				document.getElementById('cbid.simman2.%s.base_st_bw'.format(section_id)).textContent = t.band || 'n/a';
			});
		})

		return m.render();
	}
});

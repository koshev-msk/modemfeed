'use strict';
'require rpc';
'require uci';
'require ui';
'require view';
'require form';
'require poll';

var adcRequestAll = rpc.declare({
	object: 'iolines',
	method: 'voltage',
	params: [ 'adc' ],
	expect: { '': {} }
});

var DummyValueExt = form.DummyValue.extend({
	renderWidget: function(section_id, option_index, cfgvalue) {
		return E([], [
			E('div', {
				'class': 'cbi-value-field',
				'id': this.cbid(uci.get('iolines', section_id, 'dev')),
				'style': 'color:#c73d3d;margin-left:0'
			})
		]);
	}
});

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('iolines', _('Universal I/O lines'));
		m.description = _('ADC input: voltage measurement.<br />Dry contact input: when turned on, the analog input of the router is connected to +5V voltage via a pull-up resistor. The pull-up is required to ensure the correct operation of the dry contact input.<br />Open collector: open collector output is set into the active state.');

		s = m.section(form.TableSection, 'io');
		s.anonymous = true;

		o = s.option(form.DummyValue, 'name', _('I/O port'));

		o = s.option(form.ListValue, 'mode', _('Mode'));
		o.default = "mode1";
  		o.value("mode1", _("ADC"));
  		o.value("mode2", _("Dry contact"));
  		o.value("mode3", _("Open collector (OC)"));

		o = s.option(form.Flag, 'enabled', _('Save configuration before reboot'));

		o = s.option(DummyValueExt, 'voltage', _('Measured voltage, mV'));

		return m.render().then(function(mapEl) {
			poll.add(function() {
				return adcRequestAll("all").then(function(t) {
					var sections = uci.sections('iolines','io');
					for (var i = 0; i < sections.length; i++) {
						var volt_num = 'voltage'+i;
						document.getElementById('cbid.iolines.adc%s.voltage'.format(i)).textContent = t[volt_num] || 'n/a';
					}
					// document.getElementById('cbid.iolines.adc1.voltage').textContent = t.voltage1 || 'n/a';
					// document.getElementById('cbid.iolines.adc2.voltage').textContent = t.voltage2 || 'n/a';
					// document.getElementById('cbid.iolines.adc3.voltage').textContent = t.voltage3 || 'n/a';
					// document.getElementById('cbid.iolines.adc4.voltage').textContent = t.voltage4 || 'n/a';
				});
			},1);

			return mapEl;
		});
	}
});

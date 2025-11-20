'use strict';
'require form';
'require rpc';
'require fs';
'require view';
'require uci';
'require ui';
'require tools.widgets as widgets'

return view.extend({
	load: function() {
		return uci.load('smstools3');
	},

	render: function(data) {
		var m, s, o;

		m = new form.Map('smstools3', _('Smstools3: Commands'));
		m.description = _('Command List interface.');

		// Root Phone numbers
		s = m.section(form.TypedSection, 'root_phone', _('Phone'), _('Root Phone list to accept commands.'));
		s.rmempty = true;
		s.anonymous = true;

		o = s.option(form.DynamicList, 'phone', _('Phone'), _('Phone number must be without \"+\"'));
		o.rmempty = true;

		// Grid Section modems
		s = m.section(form.GridSection, 'command', _('Command List'));
		s.addremove = true;
		s.rmempty = true;

		// get modem list
		var modemOptions = [['', _('Any modem (default)')]];
		var sections = uci.sections('smstools3', 'modem');
		o = s.option(form.ListValue, 'modem', _('Modem'));

		for (var i = 0; i < sections.length; i++) {
			var section = sections[i];
			if (section['enable'] === '1') {
				modemOptions.push([section['.name'], '%s (%s)'.format(section['.name'], section['device'] || 'N/A')]);
			}
		}

		for (var i = 0; i < modemOptions.length; i++) {
			o.value(modemOptions[i][0], modemOptions[i][1]);
		}
		o.default = '';
		o.rmempty = true;

		o = s.option(form.Value, 'command', _('SMS Command'));
                o = s.option(form.Value, 'exec', _('Execute'));
		o = s.option(form.Flag, 'delay_en', _('Delay'));
		o = s.option(form.Value, 'delay', _('Delay in sec.'));
		o.depends('delay_en', '1');
		o = s.option(form.Flag, 'answer_en', _('Answer'));
		o = s.option(form.Value, 'answer', _('Answer MSG'));
		o.depends('answer_en', '1');

		return m.render();
	}

});

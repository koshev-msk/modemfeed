'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require tools.widgets as widgets';

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('smscontrol', _('Command over SMS'));
		m.description = _('You can make a call from the allowed phone number and then the router will execute the command');

		s = m.section(form.TableSection, 'call', _('Settings'));
		s.anonymous = true;

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.rmempty = false;
		o.editable = true;

		o = s.option(form.Flag, 'ack', _('Reply via SMS'));
		o.rmempty = true;
		o.editable = true;

		o = s.option(form.Value, 'command', _('Linux command'));
		o.rmempty =false;
		o.optional = false;
		o.editable = true;

		return m.render();
	}
});

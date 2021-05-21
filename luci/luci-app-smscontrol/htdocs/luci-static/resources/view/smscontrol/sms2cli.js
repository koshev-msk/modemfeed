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
		m.description = _('For example, if you enter \'1234\' as a password and send an SMS command \'1234;reboot\' to the router, the router will reboot');

		s = m.section(form.GridSection, 'remote', _('Settings'));
		s.anonymous = true;
		s.addremove = true;
		s.nodescriptions = true;

		o = s.option(form.Flag, 'enabled', _('Enabled'))
		o.rmempty = false

		o = s.option(form.Flag, 'ack', _('Reply via SMS'))
		o.rmempty = true

		o = s.option(form.Value, 'received', _('Message text'))
		o.rmempty =false
		o.optional = false

		o = s.option(form.Value, 'command', _('Linux command'))
		o.rmempty =false
		o.optional = false

		return m.render();
	}
});

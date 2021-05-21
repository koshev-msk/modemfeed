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
		m.description = _('For example, if you enter \'1234\' as a password and send an SMS command \'1234;CLI$ifconfig br-lan down\' to the router, the router will execute CLI-command ifconfig br-lan down');

		s = m.section(form.TableSection, 'cli', _('Settings'));
		s.anonymous = true;

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.rmempty = false;
		o.editable = true;

		o = s.option(form.Flag, 'ack', _('Reply via SMS'));
		o.rmempty = false;
		o.editable = true;

		o = s.option(form.DummyValue, 'received', _('Message text'));
		o.rmempty =false;
		o.value = 'CLI$any_cli_command_here';

		o = s.option(form.DummyValue, 'command', _('Linux command'));
		o.rmempty =false;
		o.optional = false;
		o.value = 'any_cli_command_here';

		return m.render();
	}
});

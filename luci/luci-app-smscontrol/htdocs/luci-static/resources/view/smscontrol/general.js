'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require tools.widgets as widgets';

return view.extend({
	render: function() {
		var m, s, o;

		m = new form.Map('smscontrol', _('Remote SMS Control'));
		m.description = _('Here you can send commands to router via a call or SMS');

		s = m.section(form.NamedSection, 'common', 'smscontrol');

		o = s.option(form.Flag, 'enabled', _('Enabled'));

		o = s.option(form.Value, 'pass', _('Password'));
		o.rmempty = false;
		o.optional = false;
		o.datatype = 'and(uciname,maxlength(15))';

		o = s.option(form.DynamicList, 'whitelist', _('Allowed phone numbers'), _('The phone number is set in the international format without \'+\''));
		o.datatype = 'phonedigit';
		o.cast = 'string';
		o.rmempty =true;
		o.optional = false;

		o = s.option(form.Flag, 'smscontrol_log', _('Enable sms and call log'));
		o.optional = false;

		return m.render();
	}
});

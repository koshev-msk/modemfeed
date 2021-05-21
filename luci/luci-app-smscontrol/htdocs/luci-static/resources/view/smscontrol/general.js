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
		params: [ 'general', 'enabled' ],
		expect: { result: false }
	}),

	render: function() {
		var m, s, o;
		
		m = new form.Map('smscontrol', _('Remote SMS Control'));
		m.description = _('Here you can send commands to router via a call or SMS');

		s = m.section(form.NamedSection, 'common', 'smscontrol');

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.rmempty = false;
		o.write = L.bind(function(section, value) {
			if (value == '1') {
				this.handleEnableService();
			}

			return uci.set('common', section, 'enabled', value);
		}, this);

		o = s.option(form.Value, 'pass', _('Password'));
		o.rmempty = false
		o.optional = false
		o.datatype = 'and(uciname,maxlength(15))'

		o = s.option(form.DynamicList, 'whitelist', _('Allowed phone numbers'));
		o.datatype = 'phonedigit'
		o.cast = 'string'
		o.rmempty =true

		return m.render();
	}
});

'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require tools.widgets as widgets';

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('smscontrol_log', _('Event log'));
		m.description = _('SMS and call log');

		s = m.section(form.TableSection, 'event');
		s.anonymous = true;
		s.addremove = true;
		s.rowcolors = true;
		s.addbtntitle = _('Remove all');
		s.handleAdd = function(ev) {
			var sections;
			sections = uci.sections('smscontrol_log');
			for (var i = 0; i < sections.length; i++) {
				uci.remove('smscontrol_log',sections[i]['.name']);
			}
			return this.map.save(null, true);
		};

		o = s.option(form.DummyValue, 'date', _('Date'));
		o = s.option(form.DummyValue, 'type', _('Type'));
		o = s.option(form.DummyValue, 'from', _('From'));
		o.default = "-";
		o = s.option(form.DummyValue, 'to', _('To'));
		o.default = "-";
		o = s.option(form.DummyValue, 'message', _('Message text'));
		o.default = "-";
		
		return m.render();
	}
});

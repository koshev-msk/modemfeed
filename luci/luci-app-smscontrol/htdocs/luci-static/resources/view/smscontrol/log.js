'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require tools.widgets as widgets';

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('event_log', _('Event log'));
		m.description = _('Event a call or SMS');

		s = m.section(form.TypedSection, 'common');
		s.anonymous = true;

		s = m.section(form.TableSection, 'event');
		s.anonymous = true;

		o = s.option(form.DummyValue, 'date', _('Date'));
		o = s.option(form.DummyValue, 'type', _('Type'));
		o = s.option(form.DummyValue, 'from', _('From'));
		o = s.option(form.DummyValue, 'to', _('To'));
		o = s.option(form.DummyValue, 'message', _('Message text'));

		o = s.option(form.Button, 'remove',_(' '));
		o.inputstyle = 'action important';
		o.inputtitle = _('Remove');
		o.onclick = function(section_id) {
		}
		
		return m.render();
	}
});

'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require fs';
'require ui'

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('smscontrol', _('Send SMS'));

		s = m.section(form.NamedSection, 'common', 'send');

		o = s.option(form.Value, 'to', _('To phone number'));
		o.datatype = 'phonedigit';
		o.cast = 'string';
		o.rmempty =true;
		o.optional =false;

		o = s.option(form.Value, 'msgtxt', _('Message text'));
		o.rmempty =true;
		o.optional =false;

		o = s.option(form.Button, 'sendsms',_(' '));
		o.inputstyle = 'action important';
		o.inputtitle = _('Send');
		o.onclick = function(section_id) {
			var to = document.getElementById('widget.cbid.smscontrol.common.to').value,
				msgtxt = document.getElementById('widget.cbid.smscontrol.common.msgtxt').value;
			if (to !== '') {			
				fs.exec('/usr/bin/sendsms', [String(to), String(msgtxt)]).then(function(res) {
					ui.addNotification(null, [
						E('p', [ _('SMS to "%s" with message "%s" generated and will be sent').format(to, msgtxt) ])
					]);
				}).catch(function(err) {
					ui.addNotification(null, [
						E('p', [ _('Error: '), err ])
					]);
				});
			} else {
				ui.addNotification(null, E('p', _('The phone number cannot be empty.')), 'error');
			}
		}

		return m.render();
	}
});

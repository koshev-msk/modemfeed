'use strict';
'require fs';
'require rpc';
'require uci';
'require view';
'require form';
'require ui';

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('report', _('Generate report'));
		m.description = _('Click "Generate archive" to download a tar archive of the selected options. It may take several minutes.');

		s = m.section(form.TypedSection, 'report');
		s.anonymous = true;

		o = s.option(form.Flag, 'kernel', _('Kernel log:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'system', _('System log:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'network', _('Network config:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'simman', _('Simman config:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'openvpn', _('OpenVPN config:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'mwan', _('MultiWAN config:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'pollmydevice', _('Pollmydevice config:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'ntp', _('NTP config:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'smsd', _('SMS config:'));
		o.rmempty = false;

		o = s.option(form.Flag, 'snmp', _('SNMP config:'));
		o.rmempty = false;

		o = s.option(form.Button, 'dl_report',_(' '));
		o.inputstyle = 'action important';
		o.inputtitle = _('Generate archive');
		o.onclick = function(ev) {
			fs.exec('/bin/report',null).then(function(res) {
				var form = E('form', {
					'method': 'post',
					'action': L.env.cgi_base + '/cgi-download',
					'enctype': 'application/x-www-form-urlencoded'
				}, [
					E('input', { 'type': 'hidden', 'name': 'sessionid', 'value': rpc.getSessionID() }),
					E('input', { 'type': 'hidden', 'name': 'path',      'value': '/tmp/report.tar.gz' }),
					E('input', { 'type': 'hidden', 'name': 'filename',  'value': 'report.tar.gz' })
				]);
				ev.target.parentNode.appendChild(form);
				form.submit();
				form.parentNode.removeChild(form);
			},this,ev.target);
		}

		return m.render();
	},
});

'use strict';
'require view';
'require form';
'require rpc';

var callServiceList = rpc.declare({
	object: 'service',
	method: 'list',
	params: [ 'name' ],
	expect: { '': {} }
});

function isRunning(res) {
	try {
		return Object.keys(res['vless-core'].instances).length > 0;
	} catch (e) {
		return false;
	}
}

return view.extend({
	load: function() {
		return L.resolveDefault(callServiceList('vless-core'), {});
	},

	render: function(res) {
		var m, s, o;

		m = new form.Map('vless-core', _('VLESS Core'),
			_('Standalone client for VLESS, Trojan, VMess, Shadowsocks and SOCKS5 upstreams with a local SOCKS5 proxy.') + '<br />' +
			'<b>' + _('Status') + ':</b> ' +
			(isRunning(res)
				? '<span style="color:green">' + _('running') + '</span>'
				: '<span style="color:red">' + _('stopped') + '</span>'));

		s = m.section(form.NamedSection, 'main', 'vless-core');
		s.addremove = false;

		o = s.option(form.Flag, 'enabled', _('Enable'));
		o.rmempty = false;

		o = s.option(form.Value, 'uri', _('Upstream URI'),
			_('<code>vless://</code>, <code>vmess://</code>, <code>trojan://</code>, ' +
			'<code>ss://</code> or <code>socks5://</code> URI of the upstream server.'));
		o.password = true;
		o.rmempty = false;
		o.validate = function(section_id, value) {
			if (value && !/^(vless|vmess|trojan|ss|socks5):\/\//.test(value))
				return _('Unsupported URI scheme');
			return true;
		};

		o = s.option(form.Value, 'listen_port', _('SOCKS5 port'),
			_('Local SOCKS5 listener, binds to 127.0.0.1 only.'));
		o.datatype = 'port';
		o.placeholder = '1080';

		o = s.option(form.Value, 'routing', _('Routing'),
			_('Optional. Format: <code>ENABLED;DEFAULT;LAN;action,type,value;...</code><br />' +
			'DEFAULT and action: <code>proxy</code>, <code>direct</code>, <code>block</code>. ' +
			'Type: <code>domain</code>, <code>suffix</code>, <code>cidr</code>, <code>port</code>.<br />' +
			'Example: <code>1;proxy;1;direct,suffix,ru;block,port,25</code>'));
		o.optional = true;

		o = s.option(form.ListValue, 'tls_mode', _('Certificate verification'),
			_('<code>strict</code>: verified TLS only (needs <code>ca-bundle</code> for security=tls). ' +
			'<code>insecure</code>: never verify. ' +
			'<code>tofu</code>: pin the certificate on first connection.'));
		o.value('strict', _('strict'));
		o.value('insecure', _('insecure'));
		o.value('tofu', _('tofu'));
		o.default = 'strict';

		return m.render();
	}
});

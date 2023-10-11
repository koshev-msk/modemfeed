'use strict';
'require view';
'require ui';
'require rpc';
'require uci';
'require form';
'require fs';
'require network';
'require firewall as fwmodel';
'require tools.firewall as fwtool';
'require tools.widgets as widgets';

var briefInfo = _('Firewall restart required. <br>In Method proxy Proxy server must be configured in transparent mode on port 3128 tcp.<br>Disable masquerade recommened.');

var rebootButton = E('button', {
        'class': 'btn cbi-button cbi-button-neutral',
        'click': ui.createHandlerFn(this, function() {
       	        return handleAction('fwrestart');
        }),
}, _('Restart'));

var FWrestart = form.DummyValue.extend({
        load: function() {
       	        var setupButton = E('button', {
               	                'class': 'cbi-button cbi-button-neutral',
                                'click': ui.createHandlerFn(this, function() {
       	                                                return handleAction('reload');
               	                                }),
                        }, _('Restart Firewall'));
       	        return L.resolveDefault(fs.exec_direct('/etc/init.d/firewall'), ['restart']).then(L.bind(function(html) {
               	        this.default = E([
                       	        E('div', { 'class': 'cbi-value' }, [
                               	                E('label', { 'class': 'cbi-value-title' },
                                       	                _('Restart Firewall')
                                                ),
       	                                        E('div', { 'class': 'cbi-value-field', 'style': 'width:25vw' },
               	                                                E('div', { 'class': 'cbi-section-node' }, [
                       	                                                rebootButton,
                               	                                ]),
                                                ),
       	                                ]),
       	                        ]);
               	}, this));
        }
});

function handleAction(ev) {
	if (ev === 'fwrestart') {
		fs.exec('/etc/init.d/firewall', ['restart']);
	}
}

return view.extend({
	render: function() {
		var m, s, o;

		m = new form.Map('ttl', _('Antitetheting Config'),
			briefInfo);

		s = m.section(form.TypedSection, 'fw');
		s.anonymous = true;
		o = s.option(FWrestart);

		s = m.section(form.TypedSection, 'ttl', _('TTL or Proxy antitether'));
		s.anonymous = true;
		s.addremove = true;

		o = s.option(widgets.NetworkSelect, 'iface', _('Set interface'));
		o.exclude = s.section;
		o.nocreate = true;
		o.optional = true;

		o = s.option(form.ListValue, 'method', _('Method'),
			_('TTL method outgoing interface<br />Proxy method incoming interfcace'));
		o.value('ttl', 'TTL');
		o.value('proxy', 'Proxy');

		o = s.option(form.Flag, 'advanced', _('Advanced Option'));
		o.default = '0';
		o.rmempty = false;

		o = s.option(form.ListValue, 'inet', _('Inet Family'));
		o.value('ipv4', 'IPv4');
		o.value('ipv6', 'IPv6');
		o.value('ipv4v6', _('Both'));
		o.rmempty = true;
		o.editable = true;
		o.depends('advanced', '1');

		o = s.option(form.Value, 'ttl', _('TTL Value'),
			_('Select TTL value. Range 1 - 255'));
		o.value('64','64')
		o.value('128','128')
		o.default = '64';
		o.rmempty = true;
		o.editable = true;
		o.depends({advanced: '1', method: /ttl/});

		o = s.option(form.Value, 'ports', _('Ports'),
			_('Incoming ports route to proxy-server<br />Custom ports range: 0-65535'));
		o.editable = true;
		o.rmempty = true;
		o.value('all', _('ALL Ports'));
		o.value('http', _('HTTP Ports'));
		o.default = 'all';
		o.depends({advanced: '1', method: /proxy/})

		return m.render();
	},
});

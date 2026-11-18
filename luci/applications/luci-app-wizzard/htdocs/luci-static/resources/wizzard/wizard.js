'use strict';
'require baseclass';
'require dom';
'require rpc';
'require uci';
'require ui';

var callTimezones = rpc.declare({
	object: 'luci',
	method: 'getTimezones',
	expect: { '': {} }
});

var callSetLocaltime = rpc.declare({
	object: 'luci',
	method: 'setLocaltime',
	params: [ 'localtime' ],
	expect: { result: 0 }
});

var callSetPassword = rpc.declare({
	object: 'luci',
	method: 'setPassword',
	params: [ 'username', 'password' ],
	expect: { result: 0 }
});

var callModems = rpc.declare({
	object: 'wizzard',
	method: 'modems',
	expect: { modems: [] }
});

var callCaps = rpc.declare({
	object: 'wizzard',
	method: 'caps',
	expect: { '': {} }
});

var callHooks = rpc.declare({
	object: 'wizzard',
	method: 'hooks',
	params: [ 'state' ],
	expect: { results: [] }
});

var callCountryList = rpc.declare({
	object: 'iwinfo',
	method: 'countrylist',
	params: [ 'device' ],
	expect: { results: [] }
});

var callDevices = rpc.declare({
	object: 'luci-rpc',
	method: 'getNetworkDevices',
	expect: { '': {} }
});

var callUciSet = rpc.declare({
	object: 'uci',
	method: 'set',
	params: [ 'config', 'section', 'values' ]
});

var callUciCommit = rpc.declare({
	object: 'uci',
	method: 'commit',
	params: [ 'config' ]
});

var CONFIGS = [ 'system', 'network', 'wireless', 'firewall', 'mwan3' ];
var SECRETS = [ 'key', 'password', 'pincode' ];

var CSS = [
	'.modal.wz-modal{max-width:760px;width:94%}.wz-body{max-height:60vh;overflow:auto;padding-right:.4em}',
	'.wz-steps{display:flex;align-items:center;justify-content:center;margin-bottom:1.2em}',
	'.wz-dot{width:2em;height:2em;line-height:2em;border-radius:50%;text-align:center;font-weight:bold;border:2px solid rgba(128,128,128,.5);opacity:.6}',
	'.wz-dot.done{opacity:1;border-color:var(--primary-color-high,#0069d6)}',
	'.wz-dot.active{opacity:1;color:#fff;background:var(--primary-color-high,#0069d6);border-color:var(--primary-color-high,#0069d6)}',
	'.wz-line{flex:0 0 2em;height:2px;background:rgba(128,128,128,.4)}',
	'.wz-card{padding:.2em .2em 1em}',
	'.wz-card h3:first-child{margin-top:0}',
	'.wz-skip{margin:.4em 0 1em;font-size:90%;opacity:.8}',
	'.wz-field{margin:.8em 0}',
	'.wz-field>label{display:block;font-weight:bold;margin-bottom:.25em}',
	'.wz-field input.cbi-input-text,.wz-field select{width:100%;box-sizing:border-box}',
	'.wz-hint{font-size:90%;opacity:.7;margin-top:.2em}',
	'.wz-error{border:1px solid #c44;border-radius:6px;padding:.6em 1em;margin-bottom:1em;background:rgba(204,68,68,.12)}',
	'.wz-modes{display:flex;gap:.8em;flex-wrap:wrap}',
	'.wz-mode{flex:1 1 150px;border:2px solid rgba(128,128,128,.35);border-radius:8px;padding:1em;cursor:pointer;text-align:center}',
	'.wz-mode.active{border-color:var(--primary-color-high,#0069d6);background:rgba(0,105,214,.08)}',
	'.wz-mode.disabled{opacity:.45;cursor:not-allowed}',
	'.wz-mode strong{display:block;font-size:110%;margin-bottom:.3em}',
	'.wz-nav{display:flex;justify-content:space-between;gap:.5em}',
	'.wz-nav>div{display:flex;gap:.5em}',
	'.wz-pre{max-height:40vh;overflow:auto;font-size:85%;white-space:pre-wrap}'
].join('\n');

function put(cfg, sid, opts) {
	for (var k in opts) {
		var v = opts[k];

		if (v == null || v === '' || (Array.isArray(v) && !v.length))
			uci.unset(cfg, sid, k);
		else
			uci.set(cfg, sid, k, v);
	}
}

function ensure(cfg, sid, type) {
	if (!uci.get(cfg, sid))
		uci.add(cfg, type, sid);
}

function field(label, node, hint) {
	return E('div', { 'class': 'wz-field' }, [
		E('label', {}, label),
		node,
		hint ? E('div', { 'class': 'wz-hint' }, hint) : ''
	]);
}

function textInput(obj, key, opts) {
	opts = opts || {};

	return E('input', {
		'class': 'cbi-input-text',
		type: opts.type || 'text',
		value: obj[key] || '',
		placeholder: opts.placeholder || '',
		maxlength: opts.maxlength,
		autocomplete: opts.type === 'password' ? 'new-password' : 'off',
		input: function(ev) {
			obj[key] = opts.type === 'password' ? ev.target.value : ev.target.value.trim();
		}
	});
}

function selectInput(obj, key, choices, cb) {
	return E('select', {
		'class': 'cbi-input-select',
		change: function(ev) {
			obj[key] = ev.target.value;
			if (cb) cb();
		}
	}, choices.map(function(c) {
		return E('option', c[0] === obj[key] ? { value: c[0], selected: 'selected' } : { value: c[0] }, c[1]);
	}));
}

function checkbox(obj, key, label, cb) {
	var cbx = E('input', {
		type: 'checkbox',
		change: function(ev) {
			obj[key] = ev.target.checked;
			if (cb) cb();
		}
	});

	cbx.checked = !!obj[key];

	return E('label', { style: 'display:block;margin:.3em 0' }, [ cbx, ' ', label ]);
}

function setFlags(values) {
	return callUciSet('wizzard', 'main', values)
		.then(function() { return callUciCommit('wizzard'); })
		.then(function() { uci.unload('wizzard'); return uci.load('wizzard'); });
}

function getFlags() {
	return L.resolveDefault(uci.load('wizzard'), null).then(function() {
		return {
			present: uci.sections('wizzard').length > 0,
			done: uci.get('wizzard', 'main', 'done') === '1',
			hide: uci.get('wizzard', 'main', 'hide') === '1'
		};
	});
}

function loadConfigs() {
	return Promise.all([
		uci.load([ 'system', 'network' ]),
		L.resolveDefault(uci.load('wireless'), null),
		L.resolveDefault(uci.load('firewall'), null),
		L.resolveDefault(uci.load('mwan3'), null)
	]).then(function(r) {
		return { wireless: !!r[1], firewall: !!r[2], mwan3: !!r[3] };
	});
}

/*
 * The user agent string carries no country, the first browser language does:
 * an explicit region (ru-RU) is used as is, a bare language (ru) only when it
 * maps to one country. Languages spread over many countries give no guess.
 */
function browserCountry(langs) {
	langs = langs || navigator.languages;

	var lang = (langs && langs[0]) || navigator.language || '';
	var m = /^[A-Za-z]{2,3}[-_]([A-Za-z]{2})(?:$|[-_])/.exec(lang);

	if (m)
		return m[1].toUpperCase();

	if (/^(en|es|pt|fr|ar|zh)$/i.test(lang))
		return '';

	try {
		return new Intl.Locale(lang).maximize().region || '';
	}
	catch (e) {
		return '';
	}
}

function safeZone(name) {
	name = (name || '').replace(/ /g, '_');

	try {
		new Intl.DateTimeFormat('en-US', { timeZone: name });
		return name;
	}
	catch (e) {
		return undefined;
	}
}

/* wall clock of the given epoch (ms) in the zone, as ms since 1970 "as if UTC" */
function wallMs(epochMs, zone) {
	var p = {};

	new Intl.DateTimeFormat('en-US', {
		timeZone: zone, hourCycle: 'h23',
		year: 'numeric', month: '2-digit', day: '2-digit',
		hour: '2-digit', minute: '2-digit', second: '2-digit'
	}).formatToParts(new Date(epochMs)).forEach(function(x) { p[x.type] = +x.value; });

	return Date.UTC(p.year, p.month - 1, p.day, p.hour, p.minute, p.second);
}

/* value for <input type="datetime-local" step="1"> showing "now" in the zone */
function nowInput(zone) {
	return new Date(wallMs(Date.now(), zone)).toISOString().slice(0, 19);
}

/* epoch seconds for a wall clock string typed by the user in the zone, NaN when invalid */
function wallToEpoch(str, zone) {
	var m = /^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2})(?::(\d{2}))?$/.exec(str || '');

	if (!m)
		return NaN;

	var guess = Date.UTC(+m[1], +m[2] - 1, +m[3], +m[4], +m[5], +(m[6] || 0));
	var e = guess - (wallMs(guess, zone) - guess);

	e = guess - (wallMs(e, zone) - e);

	return Math.floor(e / 1000);
}

function isIPv4(v) {
	return /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/.test(v) &&
		v.split('.').every(function(o) { return +o <= 255; });
}

function isTunnel(u) {
	return u.type === 'wired' && (u.proto === 'pptp' || u.proto === 'l2tp');
}

/* PPTP/L2TP run over the plain wan interface, the tunnel interface is the real uplink */
function effIface(u, name) {
	return isTunnel(u) ? name + '_vpn' : name;
}

/* wan / modem for a single interface of a type, wan1 wan2 / modem1 modem2 for several */
function uplinkNames(list) {
	var total = { wired: 0, mobile: 0 }, cnt = { wired: 0, mobile: 0 };

	list.forEach(function(u) { total[u.type]++; });

	return list.map(function(u) {
		var n = ++cnt[u.type];

		return (u.type === 'wired' ? 'wan' : 'modem') + (total[u.type] > 1 ? String(n) : '');
	});
}

return baseclass.extend({
	getFlags: getFlags,
	setFlags: setFlags,

	shouldAutostart: function() {
		return getFlags().then(function(f) {
			var dismissed = false;

			try { dismissed = !!window.sessionStorage.getItem('wizzard.dismissed'); } catch (e) {}

			return f.present && !f.done && !f.hide && !dismissed;
		});
	},

	open: function() {
		return this.load().then(L.bind(this.start, this));
	},

	load: function() {
		return loadConfigs().then(function(loaded) {
			var radio = uci.sections('wireless', 'wifi-device')[0];

			return Promise.all([
				L.resolveDefault(callTimezones(), {}),
				L.resolveDefault(callCaps(), {}),
				L.resolveDefault(callModems(), []),
				L.resolveDefault(callDevices(), {}),
				getFlags(),
				radio ? L.resolveDefault(callCountryList(radio['.name']), []) : Promise.resolve([])
			]).then(function(r) {
				return { loaded: loaded, zones: r[0], caps: r[1], modems: r[2],
					devs: r[3], flags: r[4], countries: r[5] };
			});
		});
	},

	start: function(d) {
		var zones = d.zones || {}, modems = d.modems || [], devs = d.devs || {};
		var flags = d.flags, caps = d.caps || {};
		var countries = Array.isArray(d.countries) ? d.countries : [];
		var hasMM = !!caps.modemmanager;
		var hasMwan3 = !!caps.mwan3 && d.loaded.mwan3;
		var modes = [ 'wired' ].concat(hasMM ? [ 'mobile' ] : [], hasMwan3 ? [ 'balancer' ] : []);
		var sysSid = uci.sections('system', 'system')[0]['.name'];
		var root = E('div', { 'class': 'wz' });
		var idx = 0;

		var devList = Object.keys(devs).filter(function(n) {
			return n !== 'lo' && (devs[n].devtype === 'ethernet' || devs[n].devtype === 'vlan');
		});
		var wanDev = uci.get('network', 'wan', 'device') || '';

		if (wanDev)
			devList = [ wanDev ].concat(devList.filter(function(n) { return n !== wanDev; }));

		var browserTz = '';
		try { browserTz = Intl.DateTimeFormat().resolvedOptions().timeZone; } catch (e) {}

		function freeDevice(list) {
			var used = list.filter(function(u) { return u.type === 'wired'; })
				.map(function(u) { return u.device; });

			return devList.filter(function(n) { return used.indexOf(n) < 0; })[0] || '';
		}

		function freeModem(list) {
			var used = list.filter(function(u) { return u.type === 'mobile'; })
				.map(function(u) { return u.modem; });

			return modems.map(function(m) { return m.device; })
				.filter(function(n) { return used.indexOf(n) < 0; })[0] || '';
		}

		function newUplink(list, type) {
			return {
				type: type,
				device: type === 'wired' ? freeDevice(list) : '',
				proto: 'dhcp', under: 'dhcp', server: '',
				username: '', password: '',
				ipaddr: '', netmask: '255.255.255.0', gateway: '', dns: '',
				modem: type === 'mobile' ? freeModem(list) : '',
				apn: '', auth: 'none', pin: '',
				weight: 1
			};
		}

		var s = {
			hostname: uci.get('system', sysSid, 'hostname') || 'OpenWrt',
			zonename: uci.get('system', sysSid, 'zonename') || (zones[browserTz] ? browserTz : ''),
			timeMode: 'browser',
			manualTime: '',
			mode: 'wired',
			lists: { wired: [], mobile: [], balancer: [] },
			country: '',
			wifiSame: true,
			radios: [],
			password: '', password2: '',
			hide: flags.hide,
			error: null
		};

		uci.sections('wireless', 'wifi-device').forEach(function(r) {
			var ap = uci.sections('wireless', 'wifi-iface').filter(function(i) {
				return i.device === r['.name'] && i.mode === 'ap';
			})[0];
			var bands = { '2g': '2.4 GHz', '5g': '5 GHz', '6g': '6 GHz', '60g': '60 GHz' };

			if (r.country && !s.country)
				s.country = r.country;

			s.radios.push({
				name: r['.name'],
				label: r['.name'] + (bands[r.band] ? ' (' + bands[r.band] + ')' : ''),
				on: true,
				ssid: (ap && ap.ssid) || 'OpenWrt',
				enc: 'psk2',
				key: ''
			});
		});

		var countryGuess = '';

		if (!s.country && s.radios.length) {
			var bc = browserCountry();

			if (bc && (!countries.length || countries.some(function(c) { return c.iso3166 === bc; })))
				s.country = countryGuess = bc;
		}

		/* same SSID, encryption and key on two or more enabled radios -> band steering */
		function bandSteering() {
			var list = s.radios.filter(function(r) { return r.on; }).map(function(r) {
				return s.wifiSame ? s.radios[0] : r;
			});

			return !!caps.wpad_full && list.length > 1 && list.every(function(e) {
				return e.ssid === list[0].ssid && e.enc === list[0].enc && e.key === list[0].key;
			});
		}

		function clockZone() {
			return safeZone(s.zonename) || safeZone(browserTz);
		}

		function clockEpoch() {
			if (s.timeMode === 'browser')
				return Math.floor(Date.now() / 1000);

			if (s.timeMode === 'manual')
				return wallToEpoch(s.manualTime, clockZone());

			return null;
		}

		s.lists.wired.push(newUplink(s.lists.wired, 'wired'));

		if (hasMM)
			s.lists.mobile.push(newUplink(s.lists.mobile, 'mobile'));

		if (hasMwan3) {
			var bl = s.lists.balancer;

			bl.push(newUplink(bl, 'wired'));
			bl.push(newUplink(bl, hasMM && modems.length ? 'mobile' : 'wired'));
		}

		function curUplinks() {
			return s.lists[s.mode];
		}

		function minUplinks() {
			return s.mode === 'balancer' ? 2 : 1;
		}

		function maxUplinks() {
			if (s.mode === 'mobile')
				return Math.max(modems.length, 1);

			if (s.mode === 'balancer')
				return Math.max(2, devList.length + (hasMM ? modems.length : 0));

			return 1;
		}

		/* ---------- step bodies ---------- */

		function uplinkForm(u, list, i) {
			var box = [];
			var multi = list.length > 1 || s.mode === 'balancer';

			if (multi)
				box.push(E('h3', { style: 'display:flex;justify-content:space-between;align-items:center' }, [
					(s.mode === 'mobile' ? _('Modem') : _('Connection')) + ' ' + (i + 1),
					i >= minUplinks() ? E('button', {
						'class': 'cbi-button cbi-button-remove',
						click: function() { list.splice(i, 1); draw(); }
					}, _('Remove')) : ''
				]));

			if (s.mode === 'balancer' && hasMM)
				box.push(field(_('Connection type'),
					selectInput(u, 'type', [ [ 'wired', _('Wired') ], [ 'mobile', _('Mobile (3G/4G/5G)') ] ], function() {
						var others = list.filter(function(x) { return x !== u; });

						if (u.type === 'mobile' && !u.modem)
							u.modem = freeModem(others);

						if (u.type === 'wired' && !u.device)
							u.device = freeDevice(others);

						draw();
					})));

			if (u.type === 'wired') {
				box.push(field(_('Port'), selectInput(u, 'device',
					devList.map(function(n) { return [ n, n ]; }))));
				var protos = [
					[ 'dhcp', _('Automatic (DHCP)') ],
					[ 'pppoe', _('PPPoE') ],
					[ 'static', _('Static address') ]
				];
				var tun = isTunnel(u), b = tun ? u.under : u.proto;

				if (caps.pptp)
					protos.push([ 'pptp', 'PPTP' ]);

				if (caps.l2tp)
					protos.push([ 'l2tp', 'L2TP' ]);

				box.push(field(_('Protocol'), selectInput(u, 'proto', protos, draw)));

				if (tun)
					box.push(field(_('Provider network'), selectInput(u, 'under', [
						[ 'dhcp', _('Automatic (DHCP)') ],
						[ 'static', _('Static address') ]
					], draw), _('Local connection that reaches the VPN server.')));

				if (b === 'static') {
					box.push(field(_('IPv4 address'), textInput(u, 'ipaddr', { placeholder: '192.168.100.2' })));
					box.push(field(_('Netmask'), textInput(u, 'netmask')));
					box.push(field(_('Gateway'), textInput(u, 'gateway')));
					box.push(field(_('DNS servers'), textInput(u, 'dns', { placeholder: '1.1.1.1 8.8.8.8' })));
				}

				if (u.proto === 'pppoe') {
					box.push(field(_('PPPoE login'), textInput(u, 'username')));
					box.push(field(_('PPPoE password'), textInput(u, 'password', { type: 'password' })));
				}

				if (tun) {
					box.push(field(_('VPN server'), textInput(u, 'server', { placeholder: 'vpn.example.com' }),
						u.proto === 'l2tp' ? _('L2TP without IPsec.') : ''));
					box.push(field(_('Login'), textInput(u, 'username')));
					box.push(field(_('Password'), textInput(u, 'password', { type: 'password' })));
				}
			}
			else {
				if (modems.length)
					box.push(field(_('Modem'), selectInput(u, 'modem', modems.map(function(m) {
						return [ m.device, (m.label || '').trim() + ' (#' + m.id + ')' ];
					}))));
				else
					box.push(field(_('Modem device path'), textInput(u, 'modem'),
						_('ModemManager reports no modems. Enter the sysfs path manually.')));

				box.push(field(_('APN'), textInput(u, 'apn', { placeholder: 'internet' })));
				box.push(field(_('Authentication'), selectInput(u, 'auth', [
					[ 'none', _('None') ], [ 'pap', 'PAP' ], [ 'chap', 'CHAP' ], [ 'both', 'PAP/CHAP' ]
				], draw)));

				if (u.auth !== 'none') {
					box.push(field(_('Username'), textInput(u, 'username')));
					box.push(field(_('Password'), textInput(u, 'password', { type: 'password' })));
				}

				box.push(field(_('SIM PIN'), textInput(u, 'pin', { type: 'password' }), _('Leave empty when the SIM has no PIN.')));
			}

			if (s.mode === 'balancer')
				box.push(field(_('Weight'), E('input', {
					'class': 'cbi-input-text', type: 'number', min: 1, max: 100, value: u.weight,
					input: function(ev) { u.weight = +ev.target.value; }
				}), _('Share of traffic relative to the other connections.')));

			return E('div', {}, box);
		}

		function countryField() {
			var hint = _('Sets legal channels and transmit power.');

			if (countryGuess && s.country === countryGuess)
				hint += ' ' + _('Preselected from the browser language, please check it.');

			if (!countries.length)
				return field(_('Country code'), textInput(s, 'country', { maxlength: 2, placeholder: 'DE' }),
					_('Two letters.') + ' ' + hint);

			var ch = [ [ '', _('driver default') ] ].concat(countries.map(function(c) {
				return [ c.iso3166, c.iso3166 + ' - ' + c.country ];
			}));

			if (s.country && !ch.some(function(c) { return c[0] === s.country; }))
				ch.push([ s.country, s.country ]);

			return field(_('Country code'), selectInput(s, 'country', ch), hint);
		}

		function radioFields(r) {
			return [
				field(_('Network name (SSID)'), textInput(r, 'ssid', { maxlength: 32 })),
				field(_('Encryption'), selectInput(r, 'enc', [
					[ 'psk2', 'WPA2-PSK' ],
					[ 'sae-mixed', 'WPA2/WPA3-PSK' ],
					[ 'sae', 'WPA3-SAE' ],
					[ 'none', _('No encryption (open)') ]
				], draw)),
				r.enc !== 'none' ? field(_('Wi-Fi password'), textInput(r, 'key', { type: 'password' }),
					_('8 characters or more.')) : ''
			];
		}

		var steps = [
			{
				title: _('Welcome'),
				visible: function() { return true; },
				body: function() {
					var items = steps.filter(function(st) { return st.visible() && st.intro; });

					return [
						E('p', _('This wizard helps with the first setup of the router. It takes %d short steps:').format(items.length)),
						E('ol', {}, items.map(function(st) { return E('li', st.intro); })),
						E('p', _('Nothing is changed until the last step. The final configuration is applied with an automatic rollback, so a wrong setting will not lock you out.'))
					];
				}
			},
			{
				title: _('Name and time'),
				intro: _('Router name, time zone and clock'),
				visible: function() { return true; },
				body: function() {
					var names = Object.keys(zones).sort();

					return [
						field(_('Router name'), textInput(s, 'hostname', { maxlength: 63 })),
						field(_('Time zone'), selectInput(s, 'zonename',
							[ [ '', '-- ' + _('choose') + ' --' ] ].concat(names.map(function(n) { return [ n, n ]; })))),
						field(_('Clock'), E('div', {}, [
							selectInput(s, 'timeMode', [
								[ 'browser', _('Set from this browser') ],
								[ 'manual', _('Enter manually') ],
								[ 'ntp', _('Keep as is and use NTP') ]
							], function() {
								if (s.timeMode === 'manual' && !s.manualTime)
									s.manualTime = nowInput(clockZone());

								draw();
							}),
							s.timeMode === 'browser' ? E('div', { 'class': 'wz-hint' }, new Date().toLocaleString()) : '',
							s.timeMode === 'manual' ? E('input', {
								'class': 'cbi-input-text', type: 'datetime-local', step: 1,
								value: s.manualTime,
								input: function(ev) { s.manualTime = ev.target.value; }
							}) : '',
							s.timeMode === 'manual' ? E('div', { 'class': 'wz-hint' },
								_('Time in the selected time zone.')) : ''
						]))
					];
				},
				validate: function() {
					if (!/^[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?$/.test(s.hostname))
						return _('Router name may contain letters, digits and hyphens only.');
					if (Object.keys(zones).length && !s.zonename)
						return _('Choose a time zone.');
					if (s.timeMode === 'manual' && isNaN(clockEpoch()))
						return _('Enter a valid date and time.');
				}
			},
			{
				title: _('Operating mode'),
				intro: _('How the router connects to the internet'),
				visible: function() { return modes.length > 1; },
				body: function() {
					var defs = {
						wired: [ _('Wired'), _('Cable from the provider (DHCP, PPPoE or static)') ],
						mobile: [ _('Mobile'), _('USB or mini-PCIe modem via ModemManager') ],
						balancer: [ _('Balancer'), _('Several connections shared by mwan3') ]
					};

					return [ E('div', { 'class': 'wz-modes' }, modes.map(function(m) {
						return E('div', {
							'class': 'wz-mode' + (s.mode === m ? ' active' : ''),
							click: function() { s.mode = m; draw(); }
						}, [ E('strong', defs[m][0]), defs[m][1] ]);
					})) ];
				}
			},
			{
				title: _('Internet connection'),
				intro: _('Internet connection parameters'),
				visible: function() { return true; },
				body: function() {
					var list = curUplinks(), out = [];

					list.forEach(function(u, i) {
						if (i) out.push(E('hr'));
						out.push(uplinkForm(u, list, i));
					});

					if (list.length < maxUplinks()) {
						var type = (s.mode === 'mobile' || (hasMM && freeModem(list))) ? 'mobile' : 'wired';

						out.push(E('div', { style: 'margin-top:1em' }, E('button', {
							'class': 'cbi-button cbi-button-add',
							click: function() { list.push(newUplink(list, type)); draw(); }
						}, s.mode === 'mobile' ? _('Add modem') : _('Add connection'))));
					}

					return out;
				},
				validate: function() {
					var list = curUplinks(), seen = {};

					for (var i = 0; i < list.length; i++) {
						var u = list[i], key;

						if (u.type === 'wired') {
							if (!u.device) return _('Choose a port.');
							key = 'w:' + u.device;

							var base = isTunnel(u) ? u.under : u.proto;

							if (u.proto === 'pppoe' && !u.username) return _('Enter the PPPoE login.');
							if (isTunnel(u) && (!u.server || !u.username))
								return _('Enter the VPN server and login.');
							if (base === 'static' &&
							    (!isIPv4(u.ipaddr) || !isIPv4(u.netmask) || !isIPv4(u.gateway)))
								return _('Static address, netmask and gateway must be valid IPv4 addresses.');
						}
						else {
							if (!u.modem) return _('Choose a modem.');
							key = 'm:' + u.modem;
						}

						if (seen[key])
							return _('Each connection needs its own port or modem.');

						seen[key] = true;

						if (s.mode === 'balancer' && !(u.weight >= 1 && u.weight <= 100))
							return _('Weight must be between 1 and 100.');
					}
				}
			},
			{
				title: _('Wi-Fi'),
				intro: _('Wi-Fi'),
				visible: function() { return s.radios.length > 0; },
				body: function() {
					var out = [ countryField() ];

					if (s.radios.length > 1)
						out.push(checkbox(s, 'wifiSame', _('Same network name and password on all bands'), draw));

					if (s.wifiSame) {
						out = out.concat(radioFields(s.radios[0]));
						out.push(E('div', { 'class': 'wz-field' }, [ E('label', _('Enabled radios')) ].concat(
							s.radios.map(function(r) { return checkbox(r, 'on', r.label, draw); }),
							bandSteering() ? [ E('div', { 'class': 'wz-hint' },
								_('Band steering: 802.11k and BSS transition (802.11v) will be enabled.')) ] : [])));
					}
					else
						s.radios.forEach(function(r) {
							out.push(E('h3', {}, r.label));
							out.push(checkbox(r, 'on', _('Enabled'), draw));
							if (r.on) out = out.concat(radioFields(r));
						});

					return out;
				},
				validate: function() {
					if (s.country && !/^[A-Za-z0-9]{2}$/.test(s.country))
						return _('Country code must be two letters.');

					s.country = s.country.toUpperCase();

					var list = s.wifiSame ? [ s.radios[0] ] : s.radios;

					for (var i = 0; i < list.length; i++) {
						var r = list[i];

						if (!r.on && !s.wifiSame) continue;
						if (!r.ssid) return _('Enter the network name.');
						if (r.enc !== 'none' && r.key.length < 8)
							return _('Wi-Fi password must have at least 8 characters.');
					}
				}
			},
			{
				title: _('Password'),
				intro: _('Administrator password'),
				visible: function() { return true; },
				body: function() {
					return [
						field(_('New administrator password'), textInput(s, 'password', { type: 'password' }),
							_('Used for the web interface and SSH. Leave empty to keep the current one.')),
						field(_('Confirmation'), textInput(s, 'password2', { type: 'password' }))
					];
				},
				validate: function() {
					if (s.password && s.password.length < 8)
						return _('Password must have at least 8 characters.');
					if (s.password !== s.password2)
						return _('Passwords do not match.');
				}
			}
		];

		/* ---------- stage everything into uci ---------- */

		function stage() {
			var tz = zones[s.zonename];
			var ups = curUplinks();
			var names = uplinkNames(ups);
			var ifs = ups.map(function(u, i) { return effIface(u, names[i]); });
			var stale = [];

			put('system', sysSid, {
				hostname: s.hostname,
				zonename: tz ? s.zonename : null,
				timezone: tz ? tz.tzstring : null
			});

			if (s.timeMode === 'ntp' && uci.get('system', 'ntp'))
				uci.set('system', 'ntp', 'enabled', '1');

			ups.forEach(function(u, i) {
				var n = names[i];
				var metric = ups.length > 1 ? String(10 * (i + 1)) : null;
				var tun = isTunnel(u);
				var base = tun ? u.under : u.proto;
				var dns = (base === 'static') ? u.dns.split(/[\s,]+/).filter(Boolean) : null;

				ensure('network', n, 'interface');

				if (u.type === 'wired') {
					put('network', n, {
						proto: base, device: u.device, metric: tun ? String(100 + 10 * i) : metric,
						username: base === 'pppoe' ? u.username : null,
						password: base === 'pppoe' ? u.password : null,
						ipaddr: base === 'static' ? u.ipaddr : null,
						netmask: base === 'static' ? u.netmask : null,
						gateway: base === 'static' ? u.gateway : null,
						dns: dns, server: null,
						apn: null, allowedauth: null, pincode: null, iptype: null
					});

					if (tun) {
						ensure('network', ifs[i], 'interface');
						put('network', ifs[i], {
							proto: u.proto, server: u.server,
							username: u.username, password: u.password,
							metric: String(10 * (i + 1)), auto: '1',
							device: null, ipaddr: null, netmask: null, gateway: null, dns: null
						});
					}
					else if (uci.get('network', n + '_vpn')) {
						uci.remove('network', n + '_vpn');
						stale.push(n + '_vpn');
					}

					if (n === 'wan' && uci.get('network', 'wan6'))
						put('network', 'wan6', { device: u.proto === 'pppoe' ? '@wan' : u.device });
				}
				else {
					put('network', n, {
						proto: 'modemmanager', device: u.modem, metric: metric,
						apn: u.apn, iptype: 'ipv4',
						allowedauth: u.auth === 'none' ? null : (u.auth === 'both' ? [ 'pap', 'chap' ] : [ u.auth ]),
						username: u.auth !== 'none' ? u.username : null,
						password: u.auth !== 'none' ? u.password : null,
						pincode: u.pin,
						ipaddr: null, netmask: null, gateway: null, dns: null
					});
				}
			});

			[ [ 'wired', 'wan' ], [ 'mobile', 'modem' ] ].forEach(function(b) {
				var total = ups.filter(function(u) { return u.type === b[0]; }).length;

				if (total > 1 && uci.get('network', b[1])) {
					uci.remove('network', b[1]);
					stale.push(b[1]);
				}
			});

			var zone = uci.sections('firewall', 'zone').filter(function(z) { return z.name === 'wan'; })[0];

			if (zone) {
				var nets = L.toArray(zone.network).filter(function(n) { return stale.indexOf(n) < 0; });

				names.concat(ifs).forEach(function(n) { if (nets.indexOf(n) < 0) nets.push(n); });
				uci.set('firewall', zone['.name'], 'network', nets);
			}

			if (s.mode === 'balancer') {
				var keep = {};

				ifs.forEach(function(n) { keep[n] = true; keep['wz_m_' + n] = true; });
				keep.wz_balanced = true;
				keep.wz_default = true;

				[ 'interface', 'member', 'policy', 'rule' ].forEach(function(t) {
					uci.sections('mwan3', t).forEach(function(sec) {
						if (!keep[sec['.name']])
							uci.remove('mwan3', sec['.name']);
					});
				});

				var members = [];

				ups.forEach(function(u, i) {
					var n = ifs[i], m = 'wz_m_' + n;

					ensure('mwan3', n, 'interface');
					put('mwan3', n, {
						enabled: '1', family: 'ipv4', track_ip: [ '1.1.1.1', '8.8.8.8' ],
						reliability: '1', count: '1', timeout: '2', interval: '5', down: '3', up: '3'
					});

					ensure('mwan3', m, 'member');
					put('mwan3', m, { interface: n, metric: '1', weight: String(u.weight) });
					members.push(m);
				});

				ensure('mwan3', 'wz_balanced', 'policy');
				put('mwan3', 'wz_balanced', { use_member: members, last_resort: 'unreachable' });

				ensure('mwan3', 'wz_default', 'rule');
				put('mwan3', 'wz_default', { dest_ip: '0.0.0.0/0', family: 'ipv4', use_policy: 'wz_balanced' });
			}

			var steer = bandSteering();

			s.radios.forEach(function(r) {
				var src = s.wifiSame ? s.radios[0] : r;
				var ap = uci.sections('wireless', 'wifi-iface').filter(function(i) {
					return i.device === r.name && i.mode === 'ap';
				})[0];
				var dev = { disabled: r.on ? '0' : '1' };

				if (s.country)
					dev.country = s.country;

				put('wireless', r.name, dev);

				if (!r.on && !ap)
					return;

				var sid = ap ? ap['.name'] : uci.add('wireless', 'wifi-iface', 'wz_' + r.name);

				if (!r.on) {
					put('wireless', sid, { disabled: '1' });
					return;
				}

				var opts = {
					device: r.name, mode: 'ap', network: 'lan',
					ssid: src.ssid, encryption: src.enc,
					key: src.enc === 'none' ? null : src.key,
					disabled: '0'
				};

				if (steer) {
					opts.ieee80211k = '1';
					opts.bss_transition = '1';

					/* time zone advertisement, taken from the time zone chosen on step 2 */
					if (tz && tz.tzstring) {
						opts.time_advertisement = '2';
						opts.time_zone = tz.tzstring;
					}
				}

				put('wireless', sid, opts);
			});
		}

		function previewLines() {
			var ch = uci.changes(), lines = [];

			Object.keys(ch).forEach(function(cfg) {
				ch[cfg].forEach(function(c) {
					var op = c[0], sid = c[1], opt = c[2], val = c[3];

					if (SECRETS.indexOf(opt) >= 0)
						val = '********';

					if (op === 'add')
						lines.push('+ ' + cfg + '.' + sid + '=' + opt);
					else if (op === 'remove')
						lines.push('- ' + cfg + '.' + sid + (opt ? '.' + opt : ''));
					else if (op === 'list-add')
						lines.push('  ' + cfg + '.' + sid + '.' + opt + ' += ' + val);
					else if (op === 'list-del')
						lines.push('  ' + cfg + '.' + sid + '.' + opt + ' -= ' + val);
					else if (op === 'set')
						lines.push('  ' + cfg + '.' + sid + '.' + opt + '=' + val);
				});
			});

			return lines;
		}

		function reloadUci() {
			uci.unload(CONFIGS);

			return loadConfigs();
		}

		function hookState() {
			var ups = curUplinks(), names = uplinkNames(ups);

			return JSON.stringify({
				mode: s.mode,
				hostname: s.hostname,
				zonename: s.zonename,
				country: s.country,
				uplinks: ups.map(function(u, i) {
					return {
						name: names[i],
						iface: effIface(u, names[i]),
						type: u.type,
						proto: u.type === 'wired' ? u.proto : 'modemmanager',
						device: u.type === 'wired' ? u.device : u.modem,
						weight: u.weight
					};
				}),
				radios: s.radios.filter(function(r) { return r.on; })
					.map(function(r) { return r.name; })
			});
		}

		function doApply() {
			var tasks = [];

			ui.hideModal();

			var ts = clockEpoch();

			if (ts != null)
				tasks.push(callSetLocaltime(ts));

			if (s.password)
				tasks.push(callSetPassword('root', s.password));

			tasks.push(L.resolveDefault(setFlags({ done: '1' }), null));

			return L.resolveDefault(callHooks(hookState()), []).then(function(results) {
				results.filter(function(r) { return r.code !== 0; }).forEach(function(r) {
					ui.addNotification(null, E('p', _('Hook %s failed (exit code %d): %s')
						.format(r.name, r.code, r.output || '-')), 'warning');
				});

				return Promise.all(tasks);
			})
				.then(function() { return uci.save(); })
				.then(function() { return ui.changes.apply(true); })
				.catch(function(e) {
					ui.addNotification(null, E('p', e.message), 'danger');
				});
		}

		function finish() {
			var vis = steps.filter(function(st) { return st.visible(); });

			for (var i = 0; i < vis.length; i++) {
				var e = vis[i].validate ? vis[i].validate() : null;

				if (e) {
					idx = i;
					s.error = e;
					return draw();
				}
			}

			stage();

			ui.showModal(_('Apply configuration'), [
				E('p', _('These changes will be applied. The router rolls back automatically when the web interface becomes unreachable.')),
				E('pre', { 'class': 'wz-pre' }, previewLines().join('\n') || _('No changes')),
				E('div', { 'class': 'right' }, [
					E('button', {
						'class': 'btn cbi-button cbi-button-neutral',
						click: function() { reloadUci().then(show); }
					}, _('Cancel')),
					' ',
					E('button', {
						'class': 'btn cbi-button cbi-button-positive important',
						click: doApply
					}, _('Apply'))
				])
			]);
		}

		function draw() {
			var vis = steps.filter(function(st) { return st.visible(); });
			var st = vis[idx], last = idx === vis.length - 1;
			var dots = [];

			vis.forEach(function(x, i) {
				if (i) dots.push(E('div', { 'class': 'wz-line' }));
				dots.push(E('div', {
					'class': 'wz-dot' + (i < idx ? ' done' : '') + (i === idx ? ' active' : ''),
					title: x.title
				}, String(i + 1)));
			});

			dom.content(root, [
				E('div', { 'class': 'wz-steps' }, dots),
				s.error ? E('div', { 'class': 'wz-error' }, s.error) : '',
				E('div', { 'class': 'wz-body' }, E('div', { 'class': 'wz-card' }, [ E('h3', st.title) ].concat(st.body()))),
				E('div', { 'class': 'wz-skip' }, checkbox(s, 'hide', _('Do not show this wizard automatically next time'), function() {
					L.resolveDefault(setFlags({ hide: s.hide ? '1' : '0' }), null);
				})),
				E('div', { 'class': 'wz-nav' }, [
					E('div', {}, [
						E('button', {
							'class': 'cbi-button cbi-button-neutral',
							click: function() {
								try { window.sessionStorage.setItem('wizzard.dismissed', '1'); } catch (e) {}
								ui.hideModal();
							}
						}, _('Close')),
						idx > 0 ? E('button', {
							'class': 'cbi-button cbi-button-neutral',
							click: function() { s.error = null; idx--; draw(); }
						}, '\u2190 ' + _('Back')) : ''
					]),
					E('button', {
						'class': 'cbi-button ' + (last ? 'cbi-button-positive' : 'cbi-button-action'),
						click: function() {
							var e = st.validate ? st.validate() : null;

							if (e) { s.error = e; return draw(); }

							s.error = null;

							if (last) return finish();

							idx++;
							draw();
						}
					}, last ? _('Finish') : _('Next') + ' \u2192')
				])
			]);
		}

		function show() {
			ui.showModal(_('Setup Wizard'), [ E('style', CSS), root ], 'wz-modal');
			draw();
		}

		show();
	}
});

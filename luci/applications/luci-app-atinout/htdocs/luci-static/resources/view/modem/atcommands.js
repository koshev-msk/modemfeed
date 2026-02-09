'use strict';
'require dom';
'require form';
'require fs';
'require ui';
'require view';

/*
	Copyright 2022-2023 Rafał Wabik - IceG - From eko.one.pl forum

	Modified for atinout by Konstantine Shevlakov <shevlakov@132lan.ru> 2025

	Licensed to the GNU General Public License v3.0.
*/

return view.extend({
	handleCommand: function(exec, args) {
		var buttons = document.querySelectorAll('.cbi-button');

		for (var i = 0; i < buttons.length; i++)
			buttons[i].setAttribute('disabled', 'true');

		return fs.exec(exec, args).then(function(res) {
			var out = document.querySelector('.atcommand-output');
			out.style.display = '';

			res.stdout = res.stdout?.trim().replace(/\n\s*\n\s*\n+/g, '\n\n') || '';
			res.stderr = res.stderr?.trim().replace(/\n\s*\n\s*\n+/g, '\n\n') || '';

			dom.content(out, [ res.stdout || '', res.stderr || '' ]);

		}).catch(function(err) {
			ui.addNotification(null, E('p', [ err ]))
		}).finally(function() {
			for (var i = 0; i < buttons.length; i++)
			buttons[i].removeAttribute('disabled');

		});
	},

	handleGo: function(ev) {
		var atcmd = document.getElementById('cmdvalue').value;
		var portSelect = document.getElementById('portselect');
		var port = portSelect ? portSelect.value : '';

		if (atcmd.length < 2) {
			ui.addNotification(null, E('p', _('Please specify the AT command to send')), 'info');
			return false;
		}

		if (!port) {
			ui.addNotification(null, E('p', _('Please select a port for communication with the modem')), 'info');
			return false;
		}

		return this.handleCommand('luci-app-atinout', [atcmd, port]);
	},

	handleClear: function(ev) {
		var out = document.querySelector('.atcommand-output');
		out.style.display = 'none';

		var ov = document.getElementById('cmdvalue');
		ov.value = '';

		document.getElementById('cmdvalue').focus();
	},

	handleCopy: function(ev) {
		var ov = document.getElementById('cmdvalue');
		ov.value = '';
		var x = document.getElementById('tk').value;
		ov.value = x;
	},

	// search port in /dev/ dir
	scanPorts: function() {
		return fs.list('/dev').then(function(devices) {
			var ports = [];

			if (devices) {
				devices.forEach(function(device) {
					var name = device.name;
					if (name) {
						if (name.startsWith('ttyUSB') ||
							name.startsWith('ttyACM') ||
							/^wwan\d+at\d+/.test(name)) {
							ports.push('/dev/' + name);
						}
					}
				});
			}

			ports.sort();
			return ports;
		}).catch(function(err) {
			console.error('Error scanning ports:', err);
			return [];
		});
	},

	updatePortList: function() {
		var self = this;
		var portSelect = document.getElementById('portselect');

		if (!portSelect) return;

		portSelect.disabled = true;
		var currentValue = portSelect.value;

		this.scanPorts().then(function(ports) {
			var currentPort = currentValue;

			while (portSelect.firstChild) {
				portSelect.removeChild(portSelect.firstChild);
			}

			var emptyOption = E('option', { value: '' }, _('-- Select port --'));
			portSelect.appendChild(emptyOption);

			ports.forEach(function(port) {
				var option = E('option', { value: port }, port);
				if (port === currentPort) {
					option.setAttribute('selected', 'selected');
				}
				portSelect.appendChild(option);
			});

			portSelect.disabled = false;

			// if ports not found
			if (ports.length === 0) {
				ui.addNotification(null, E('p', _('No ttyUSB or ttyACM ports found')), 'info');
			}
		});
	},

	load: function() {
		return Promise.all([
			L.resolveDefault(fs.read_direct('/etc/atcommands.user'), null)
		]);
	},

	render: function (loadResults) {
		var info = _('User interface for handling AT commands using atinout utility.');
		var self = this;

		var container = E('div', { 'class': 'cbi-map', 'id': 'map' }, [
			E('h2', {}, [ _('AT Commands') ]),
			E('div', { 'class': 'cbi-map-descr'}, info),
			E('hr'),
			E('div', { 'class': 'cbi-section' }, [
				E('div', { 'class': 'cbi-section-node' }, [
					E('div', { 'class': 'cbi-value' }, [
						E('label', { 'class': 'cbi-value-title' }, [ _('User AT commands') ]),
						E('div', { 'class': 'cbi-value-field' }, [
							E('select', { 
								'class': 'cbi-input-select', 
								'id': 'tk', 
								'style': 'margin:5px 0; width:100%;', 
								'change': ui.createHandlerFn(this, 'handleCopy')
							},
								(loadResults[0] || "").trim().split("\n").map(function(cmd) {
									var fields = cmd.split(/;/);
									var name = fields[0];
									var code = fields[1];
									return E('option', { 'value': code }, name );
								})
							)
						]) 
					]),

					E('div', { 'class': 'cbi-value' }, [
						E('label', { 'class': 'cbi-value-title' }, [ _('Modem Port') ]),
						E('div', { 'class': 'cbi-value-field' }, [
							E('div', { 'style': 'display: flex; align-items: center; gap: 10px;' }, [
								E('select', {
									'class': 'cbi-input-select',
									'id': 'portselect',
									'style': 'margin:5px 0; width: 100%;',
									'name': 'port'
								}, [
									E('option', { value: '' }, _('-- Scanning ports --'))
								]),
								E('button', {
									'class': 'cbi-button cbi-button-action',
									'style': 'white-space: nowrap;',
									'click': ui.createHandlerFn(this, function() {
										this.updatePortList();
									})
								}, [ _('Refresh') ])
							])
						])
					]),

					E('div', { 'class': 'cbi-value' }, [
						E('label', { 'class': 'cbi-value-title' }, [ _('Command to send') ]),
						E('div', { 'class': 'cbi-value-field' }, [
							E('input', {
								'style': 'margin:5px 0; width:100%;',
								'type': 'text',
								'id': 'cmdvalue',
								'data-tooltip': _('Press [Enter] to send the command, press [Delete] to delete the command'),
								'keydown': function(ev) {
									if (ev.keyCode === 13) {
										var execBtn = document.getElementById('execute');
										if (execBtn)
											execBtn.click();
									}
									if (ev.keyCode === 46) {
										var del = document.getElementById('cmdvalue');
										if (del) {
											var ov = document.getElementById('cmdvalue');
											ov.value = '';
											document.getElementById('cmdvalue').focus();
										}
									}
								}
							}),
						])
					]),
				])
			]),
			E('hr'),
			E('div', { 'class': 'right' }, [
				E('button', {
					'class': 'cbi-button cbi-button-remove',
					'id': 'clr',
					'click': ui.createHandlerFn(this, 'handleClear')
				}, [ _('Clear form') ]),
				'\xa0\xa0\xa0',
				E('button', {
					'class': 'cbi-button cbi-button-action important',
					'id': 'execute',
					'click': ui.createHandlerFn(this, 'handleGo')
				}, [ _('Send command') ]),
			]),
			E('p', _('Reply')),
			E('pre', { 
				'class': 'atcommand-output', 
				'id': 'preout', 
				'style': 'display:none; border: 1px solid var(--border-color-medium); border-radius: 5px; font-family: monospace' 
			}),
		]);

		setTimeout(function() {
			self.updatePortList();
		}, 100);

		return container;
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});

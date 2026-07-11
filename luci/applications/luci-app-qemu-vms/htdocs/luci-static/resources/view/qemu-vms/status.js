'use strict';
'require view';
'require rpc';
'require poll';
'require ui';
'require dom';
'require uci';

var callStatus = rpc.declare({
	object: 'luci.qemu-vms',
	method: 'status',
	params: ['vm_name']
});

var callStart = rpc.declare({ object: 'luci.qemu-vms', method: 'start', params: ['vm_name'] });
var callStop = rpc.declare({ object: 'luci.qemu-vms', method: 'stop', params: ['vm_name'] });
var callRestart = rpc.declare({ object: 'luci.qemu-vms', method: 'restart', params: ['vm_name'] });
var callReadlog = rpc.declare({ object: 'luci.qemu-vms', method: 'readlog', params: ['vm_name', 'lines'] });
var callConsoleOpen = rpc.declare({ object: 'luci.qemu-vms', method: 'console_open', params: ['vm_name'] });
var callConsoleClose = rpc.declare({ object: 'luci.qemu-vms', method: 'console_close', params: ['vm_name'] });
var callDeleteVm = rpc.declare({ object: 'luci.qemu-vms', method: 'delete_vm', params: ['vm_name'] });

function fmtMB(kb) {
	return kb ? (kb / 1024).toFixed(1) + ' MB' : '-';
}

// LuCI's default .modal has a fairly narrow max-width from the theme CSS,
// which leaves a lot of dead space around a VNC/terminal iframe. Widen the
// dialog after it's been inserted into the DOM (ui.showModal itself takes
// no width option).
function widenModal() {
	var modal = document.querySelector('.modal');
	if (modal) {
		modal.style.width = '92vw';
		modal.style.maxWidth = '1400px';
	}
}

return view.extend({
	load: function() {
		return Promise.all([
			callStatus(),
			uci.load('qemu-vms')
		]);
	},

	renderCard: function(name, vm) {
		var self = this;

		return E('div', {
			'class': 'cbi-section',
			'id': 'vm-card-%s'.format(name),
			'data-vm': name
		}, [
			E('h3', {}, [
				name + ' ',
				E('span', {
					'id': 'vm-label-%s'.format(name),
					'class': 'label ' + (vm.running ? 'success' : 'warning')
				}, vm.running ? _('running') : _('stopped')),
				vm.anonymous ? ' \u26A0\uFE0F' : ''
			]),
			vm.anonymous ? E('p', { 'class': 'alert-message warning' },
				_('This VM section has no explicit name in UCI. Paths are derived from an auto-generated id and may change.')) : '',
			E('table', { 'class': 'table' }, [
				E('tr', { 'class': 'tr' }, [
					E('td', { 'class': 'td left', 'width': '30%' }, _('PID')),
					E('td', { 'class': 'td left', 'id': 'vm-pid-%s'.format(name) }, vm.pid || '-')
				]),
				E('tr', { 'class': 'tr' }, [
					E('td', { 'class': 'td left' }, _('RAM (host RSS)')),
					E('td', { 'class': 'td left', 'id': 'vm-ram-%s'.format(name) }, fmtMB(vm.ram_kb))
				]),
				E('tr', { 'class': 'tr' }, [
					E('td', { 'class': 'td left' }, _('Disk image size')),
					E('td', { 'class': 'td left', 'id': 'vm-disk-%s'.format(name) },
						vm.disk_bytes ? (vm.disk_bytes / 1048576).toFixed(0) + ' MB' : '-')
				])
			]),
			E('div', { 'class': 'cbi-page-actions' }, [
				E('button', {
					'id': 'vm-btn-start-%s'.format(name),
					'class': 'btn cbi-button cbi-button-positive',
					'disabled': vm.running || null,
					'click': ui.createHandlerFn(self, 'handleStart', name)
				}, _('Start')),
				E('button', {
					'id': 'vm-btn-stop-%s'.format(name),
					'class': 'btn cbi-button cbi-button-negative',
					'disabled': vm.running ? null : true,
					'click': ui.createHandlerFn(self, 'handleStop', name)
				}, _('Stop')),
				E('button', {
					'id': 'vm-btn-restart-%s'.format(name),
					'class': 'btn cbi-button',
					'disabled': vm.running ? null : true,
					'click': ui.createHandlerFn(self, 'handleRestart', name)
				}, _('Restart')),
				E('button', {
					'id': 'vm-btn-console-%s'.format(name),
					'class': 'btn cbi-button',
					'disabled': vm.running ? null : true,
					'click': ui.createHandlerFn(self, 'handleConsole', name)
				}, _('Console')),
				E('button', {
					'class': 'btn cbi-button',
					'click': ui.createHandlerFn(self, 'handleLog', name)
				}, _('Log')),
				E('button', {
					'class': 'btn cbi-button',
					'click': ui.createHandlerFn(self, 'handleEdit', name)
				}, _('Edit')),
				E('button', {
					'class': 'btn cbi-button-remove',
					'click': ui.createHandlerFn(self, 'handleDelete', name)
				}, _('Delete'))
			])
		]);
	},

	render: function(data) {
		var vms = data[0];
		var self = this;
		var container = E('div', {});

		container.appendChild(E('div', { 'class': 'cbi-page-actions', 'style': 'margin-bottom: 1em' }, [
			E('button', {
				'class': 'btn cbi-button cbi-button-add',
				'click': ui.createHandlerFn(self, 'handleEdit', null)
			}, _('Add VM'))
		]));

		Object.keys(vms).sort().forEach(function(name) {
			container.appendChild(this.renderCard(name, vms[name]));
		}.bind(this));

		poll.add(function() {
			return callStatus().then(function(vms) {
				Object.keys(vms).forEach(function(name) {
					var vm = vms[name];

					var labelEl = document.getElementById('vm-label-%s'.format(name));
					if (labelEl) {
						labelEl.textContent = vm.running ? _('running') : _('stopped');
						labelEl.className = 'label ' + (vm.running ? 'success' : 'warning');
					}

					var pidEl = document.getElementById('vm-pid-%s'.format(name));
					if (pidEl) pidEl.textContent = vm.pid || '-';

					var ramEl = document.getElementById('vm-ram-%s'.format(name));
					if (ramEl) ramEl.textContent = fmtMB(vm.ram_kb);

					var diskEl = document.getElementById('vm-disk-%s'.format(name));
					if (diskEl) diskEl.textContent = vm.disk_bytes ? (vm.disk_bytes / 1048576).toFixed(0) + ' MB' : '-';

					var startBtn = document.getElementById('vm-btn-start-%s'.format(name));
					var stopBtn = document.getElementById('vm-btn-stop-%s'.format(name));
					var restartBtn = document.getElementById('vm-btn-restart-%s'.format(name));
					var consoleBtn = document.getElementById('vm-btn-console-%s'.format(name));

					if (startBtn) startBtn.disabled = !!vm.running;
					if (stopBtn) stopBtn.disabled = !vm.running;
					if (restartBtn) restartBtn.disabled = !vm.running;
					if (consoleBtn) consoleBtn.disabled = !vm.running;
				});
			});
		}, 3);

		return container;
	},

	handleStart: function(name, ev) {
		return callStart(name).then(function() {
			ui.addNotification(null, E('p', _('Starting %s\u2026').format(name)), 'info');
		});
	},

	handleStop: function(name, ev) {
		return callStop(name).then(function() {
			ui.addNotification(null, E('p', _('Stopping %s\u2026').format(name)), 'info');
		});
	},

	handleRestart: function(name, ev) {
		return callRestart(name).then(function() {
			ui.addNotification(null, E('p', _('Restarting %s\u2026').format(name)), 'info');
		});
	},

	handleLog: function(name, ev) {
		return callReadlog(name, 400).then(function(res) {
			ui.showModal(_('Console log: %s').format(name), [
				E('pre', {
					'style': 'max-height: 60vh; overflow: auto; white-space: pre-wrap; font-size: 12px;'
				}, res.log || _('(empty)')),
				E('div', { 'class': 'right' }, [
					E('button', {
						'class': 'btn',
						'click': ui.hideModal
					}, _('Close'))
				])
			]);
		});
	},

	handleConsole: function(name, ev) {
		var vm = uci.get('qemu-vms', name) || {};

		if (vm.display_type === 'vnc')
			return this.showVncModal(name, vm);

		return this.showSerialModal(name);
	},

	showVncModal: function(name, vm) {
		var port = parseInt(vm.vnc_port, 10);
		if (!port) {
			ui.addNotification(null, E('p', _('This VM has no VNC port configured. Edit the VM first.')), 'error');
			return;
		}

		var wsPort = port + 1000;
		var src = '/luci-static/resources/novnc/vnc.html?host=' + window.location.hostname +
			'&port=' + wsPort + '&path=&scale=true';

		ui.showModal(_('VNC console: %s').format(name), [
			E('iframe', {
				'src': src,
				'style': 'width: 100%; height: 80vh; border: 0; background: #000;'
			}),
			E('p', { 'class': 'cbi-value-description' },
				_('The VNC websocket (port %s) must be reachable from your browser \u2014 same requirement as any other port opened directly on the router.').format(wsPort)),
			E('div', { 'class': 'right', 'style': 'margin-top: 1em' }, [
				E('button', { 'class': 'btn', 'click': ui.hideModal }, _('Close'))
			])
		]);
		widenModal();
	},

	showSerialModal: function(name) {
		return callConsoleOpen(name).then(function(res) {
			if (!res.success) {
				ui.addNotification(null, E('p', res.error || _('Failed to open console')), 'error');
				return;
			}

			var src = window.location.protocol + '//' + window.location.hostname + ':' + res.port + '/';

			ui.showModal(_('Console: %s').format(name), [
				E('iframe', {
					'src': src,
					'style': 'width: 100%; height: 80vh; border: 0; background: #000;'
				}),
				E('p', { 'class': 'cbi-value-description' },
					_('ttyd console (port %s, ephemeral) must be reachable from your browser \u2014 same requirement as any other port opened directly on the router.').format(res.port)),
				E('div', { 'class': 'right', 'style': 'margin-top: 1em' }, [
					E('button', { 'class': 'btn', 'click': ui.hideModal }, _('Close'))
				])
			]);
			widenModal();
		});
	},

	handleDelete: function(name, ev) {
		ui.showModal(_('Delete %s?').format(name), [
			E('p', {}, _('This permanently removes the VM configuration from UCI. The disk image and CD-ROM files are not touched.')),
			E('div', { 'class': 'right' }, [
				E('button', { 'class': 'btn', 'click': ui.hideModal }, _('Cancel')),
				' ',
				E('button', {
					'class': 'btn cbi-button-negative',
					'click': function() {
						return callDeleteVm(name).then(function() {
							ui.hideModal();
							ui.addNotification(null, E('p', _('Deleted %s').format(name)), 'info');
							window.location.reload();
						});
					}
				}, _('Delete'))
			])
		]);
	},

	handleEdit: function(name, ev) {
		var isNew = !name;
		var existingNameForReset = isNew ? null : name;
		var vm = isNew ? {} : (uci.get('qemu-vms', name) || {});

		var allNetworks = uci.sections('qemu-vms', 'network').map(function(s) { return s['.name']; });
		var allPci = uci.sections('qemu-vms', 'pci-passthrough').map(function(s) { return s['.name']; });
		var allUsb = uci.sections('qemu-vms', 'usb-passthrough').map(function(s) { return s['.name']; });
		var attachedNetworks = [].concat(vm.network || []);
		var attachedUsb = [].concat(vm.usb_passthrough || []);

		var nameInput = E('input', {
			'class': 'cbi-input-text',
			'id': 'edit-name',
			'value': name || '',
			'disabled': isNew ? null : true,
			'placeholder': isNew ? _('e.g. ap, test, vlan') : null
		});

		var enabledInput = E('input', {
			'type': 'checkbox',
			'id': 'edit-enabled',
			'checked': (vm.enabled !== '0') || null
		});

		var memoryInput = E('input', {
			'class': 'cbi-input-text',
			'id': 'edit-memory',
			'type': 'number',
			'min': '32',
			'step': '32',
			'value': vm.memory || '128'
		});
		var smpInput = E('input', {
			'class': 'cbi-input-text',
			'id': 'edit-smp',
			'type': 'number',
			'min': '1',
			'max': '16',
			'step': '1',
			'value': vm.smp || '1'
		});

		var cpuSelect = E('select', { 'class': 'cbi-input-select', 'id': 'edit-cpu' }, [
			E('option', { 'value': 'host', 'selected': (vm.cpu || 'host') === 'host' || null }, 'host'),
			E('option', { 'value': 'max', 'selected': vm.cpu === 'max' || null }, 'max'),
			E('option', { 'value': 'qemu64', 'selected': vm.cpu === 'qemu64' || null }, 'qemu64'),
			E('option', { 'value': 'Haswell', 'selected': vm.cpu === 'Haswell' || null }, 'Haswell'),
			E('option', { 'value': 'Skylake-Client', 'selected': vm.cpu === 'Skylake-Client' || null }, 'Skylake-Client')
		]);

		var imageInput = E('input', {
			'class': 'cbi-input-text',
			'id': 'edit-image',
			'value': vm.image || '',
			'placeholder': '/mnt/disk/vm/name.img'
		});

		var diskBusSelect = E('select', { 'class': 'cbi-input-select', 'id': 'edit-disk-bus' }, [
			E('option', { 'value': 'virtio', 'selected': (vm.disk_bus || 'virtio') === 'virtio' || null }, 'virtio (default, best performance, needs guest drivers)'),
			E('option', { 'value': 'ide', 'selected': vm.disk_bus === 'ide' || null }, 'IDE (widest compatibility \u2014 old Windows/DOS/legacy kernels)'),
			E('option', { 'value': 'sata', 'selected': vm.disk_bus === 'sata' || null }, 'SATA/AHCI'),
			E('option', { 'value': 'scsi', 'selected': vm.disk_bus === 'scsi' || null }, 'virtio-SCSI')
		]);

		var cdromInput = E('input', {
			'class': 'cbi-input-text',
			'id': 'edit-cdrom',
			'value': vm.cdrom || '',
			'placeholder': _('optional, e.g. /mnt/disk/iso/installer.iso')
		});

		var existingCustomArgs = [].concat(vm.custom_arg || []);
		var customArgsInput = E('textarea', {
			'class': 'cbi-input-textarea',
			'id': 'edit-custom-args',
			'style': 'width: 100%; height: 6em; font-family: monospace;',
			'placeholder': _('one raw QEMU argument per line, e.g.\n-device usb-tablet\n-boot order=d')
		}, existingCustomArgs.join('\n'));

		var displaySelect = E('select', { 'class': 'cbi-input-select', 'id': 'edit-display' }, [
			E('option', { 'value': 'serial', 'selected': (vm.display_type || 'serial') === 'serial' || null }, _('Serial console (ttyd)')),
			E('option', { 'value': 'vnc', 'selected': vm.display_type === 'vnc' || null }, _('VNC (noVNC)'))
		]);

		var usedPorts = uci.sections('qemu-vms', 'vm')
			.map(function(s) { return parseInt(s.vnc_port, 10); })
			.filter(function(p) { return !isNaN(p); });
		var suggestedPort = usedPorts.length ? Math.max.apply(null, usedPorts) + 1 : 5901;

		var vncPortInput = E('input', {
			'class': 'cbi-input-text',
			'id': 'edit-vnc-port',
			'type': 'number',
			'min': '5900',
			'value': vm.vnc_port || suggestedPort
		});

		var vncPortRow = E('div', { 'class': 'cbi-value', 'id': 'edit-vnc-port-row' }, [
			E('label', { 'class': 'cbi-value-title' }, _('VNC port (RFB, 5900+)')),
			E('div', { 'class': 'cbi-value-field' }, [
				vncPortInput,
				E('p', { 'class': 'cbi-value-description' },
					_('Serves noVNC over websocket on port %s.').format('+1000'))
			])
		]);

		displaySelect.addEventListener('change', function() {
			vncPortRow.style.display = (displaySelect.value === 'vnc') ? '' : 'none';
		});
		vncPortRow.style.display = ((vm.display_type || 'serial') === 'vnc') ? '' : 'none';

		var pciSelect = E('select', { 'class': 'cbi-input-select', 'id': 'edit-pci' }, [
			E('option', { 'value': '', 'selected': !vm.pci_passthrough || null }, _('-- none --'))
		].concat(allPci.map(function(p) {
			return E('option', { 'value': p, 'selected': vm.pci_passthrough === p || null }, p);
		})));

		var networkChecks = allNetworks.map(function(net) {
			return E('div', {}, [
				E('label', {}, [
					E('input', {
						'type': 'checkbox',
						'class': 'edit-network-check',
						'value': net,
						'checked': attachedNetworks.indexOf(net) !== -1 || null
					}),
					' ' + net
				])
			]);
		});

		if (!allNetworks.length)
			networkChecks = [E('em', {}, _('No networks defined yet \u2014 add some on the Networks tab.'))];

		var usbSelect = E('div', {
			'style': 'display: flex; flex-wrap: wrap; gap: 0.3em 1.2em;'
		}, allUsb.map(function(usb) {
			return E('label', { 'style': 'white-space: nowrap; font-weight: normal;' }, [
				E('input', {
					'type': 'checkbox',
					'class': 'edit-usb-check',
					'value': usb,
					'checked': attachedUsb.indexOf(usb) !== -1 || null
				}),
				' ' + usb
			]);
		}));

		if (!allUsb.length)
			usbSelect = E('em', {}, _('No USB passthrough sections defined yet \u2014 add some on the Hardware passthrough tab.'));


		ui.showModal(isNew ? _('Add VM') : _('Edit %s').format(name), [
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Name')),
				E('div', { 'class': 'cbi-value-field' }, nameInput)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Enabled')),
				E('div', { 'class': 'cbi-value-field' }, enabledInput)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Memory (MB)')),
				E('div', { 'class': 'cbi-value-field' }, memoryInput)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('SMP (vCPUs)')),
				E('div', { 'class': 'cbi-value-field' }, smpInput)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('CPU model')),
				E('div', { 'class': 'cbi-value-field' }, cpuSelect)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Disk image path')),
				E('div', { 'class': 'cbi-value-field' }, imageInput)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Disk bus')),
				E('div', { 'class': 'cbi-value-field' }, diskBusSelect)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('CD-ROM (ISO)')),
				E('div', { 'class': 'cbi-value-field' }, cdromInput)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Custom QEMU arguments')),
				E('div', { 'class': 'cbi-value-field' }, [
					customArgsInput,
					E('p', { 'class': 'cbi-value-description' },
						_('Escape hatch for flags with no dedicated field \u2014 appended verbatim to the qemu-system-x86_64 command line.'))
				])
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Console type')),
				E('div', { 'class': 'cbi-value-field' }, displaySelect)
			]),
			vncPortRow,
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('PCI passthrough')),
				E('div', { 'class': 'cbi-value-field' }, pciSelect)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('USB passthrough')),
				E('div', { 'class': 'cbi-value-field' }, usbSelect)
			]),
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Attached networks')),
				E('div', { 'class': 'cbi-value-field' }, networkChecks)
			]),
			E('div', { 'class': 'right', 'style': 'margin-top: 1em' }, [
				E('button', { 'class': 'btn', 'click': ui.hideModal }, _('Cancel')),
				' ',
				E('button', {
					'class': 'btn',
					'click': ui.createHandlerFn(this, 'handleEdit', existingNameForReset)
				}, _('Reset')),
				' ',
				E('button', {
					'class': 'btn cbi-button-positive',
					'click': ui.createHandlerFn(this, 'saveEdit', isNew ? null : name)
				}, _('Save'))
			])
		]);
	},

	saveEdit: function(existingName, ev) {
		var name = existingName || document.getElementById('edit-name').value.trim();

		if (!name) {
			ui.addNotification(null, E('p', _('VM name is required')), 'error');
			return;
		}
		if (!existingName && uci.get('qemu-vms', name)) {
			ui.addNotification(null, E('p', _('A VM with this name already exists')), 'error');
			return;
		}

		if (!existingName)
			uci.add('qemu-vms', 'vm', name);

		var memoryRaw = document.getElementById('edit-memory').value.trim();
		var smpRaw = document.getElementById('edit-smp').value.trim();

		if (!/^\d+$/.test(memoryRaw)) {
			ui.addNotification(null, E('p', _('Memory must be a whole number (MB)')), 'error');
			return;
		}
		if (!/^\d+$/.test(smpRaw)) {
			ui.addNotification(null, E('p', _('SMP must be a whole number')), 'error');
			return;
		}

		var memory = parseInt(memoryRaw, 10);
		var smp = parseInt(smpRaw, 10);

		if (memory < 32) {
			ui.addNotification(null, E('p', _('Memory must be \u2265 32 MB')), 'error');
			return;
		}
		if (smp < 1 || smp > 16) {
			ui.addNotification(null, E('p', _('SMP must be between 1 and 16')), 'error');
			return;
		}

		uci.set('qemu-vms', name, 'enabled', document.getElementById('edit-enabled').checked ? '1' : '0');
		uci.set('qemu-vms', name, 'memory', String(memory));
		uci.set('qemu-vms', name, 'smp', String(smp));
		uci.set('qemu-vms', name, 'cpu', document.getElementById('edit-cpu').value);
		uci.set('qemu-vms', name, 'image', document.getElementById('edit-image').value);
		uci.set('qemu-vms', name, 'disk_bus', document.getElementById('edit-disk-bus').value);

		var displayType = document.getElementById('edit-display').value;
		uci.set('qemu-vms', name, 'display_type', displayType);

		if (displayType === 'vnc') {
			var vncPortRaw = document.getElementById('edit-vnc-port').value.trim();
			if (!/^\d+$/.test(vncPortRaw) || parseInt(vncPortRaw, 10) < 5900) {
				ui.addNotification(null, E('p', _('VNC port must be a number \u2265 5900')), 'error');
				return;
			}
			uci.set('qemu-vms', name, 'vnc_port', vncPortRaw);
		} else {
			uci.unset('qemu-vms', name, 'vnc_port');
		}

		var cdrom = document.getElementById('edit-cdrom').value.trim();
		if (cdrom)
			uci.set('qemu-vms', name, 'cdrom', cdrom);
		else
			uci.unset('qemu-vms', name, 'cdrom');

		var customArgs = document.getElementById('edit-custom-args').value
			.split('\n')
			.map(function(line) { return line.trim(); })
			.filter(function(line) { return line.length > 0; });
		if (customArgs.length)
			uci.set('qemu-vms', name, 'custom_arg', customArgs);
		else
			uci.unset('qemu-vms', name, 'custom_arg');

		var pci = document.getElementById('edit-pci').value;
		if (pci)
			uci.set('qemu-vms', name, 'pci_passthrough', pci);
		else
			uci.unset('qemu-vms', name, 'pci_passthrough');


		//var nets = Array.prototype.slice.call(document.querySelectorAll('.edit-network-check:checked'))
		//	.map(function(el) { return el.value; });
		//uci.set('qemu-vms', name, 'network', nets);

		var newUsb = Array.prototype.slice.call(document.querySelectorAll('.edit-usb-check:checked'))
			.map(function(el) { return el.value; });

		var newNets = Array.prototype.slice.call(document.querySelectorAll('.edit-network-check:checked'))
			.map(function(el) { return el.value; });

		if (existingName) {
			var oldNets = uci.get('qemu-vms', name, 'network') || [];
			if (!Array.isArray(oldNets)) oldNets = oldNets ? [oldNets] : [];

			var sortedOld = oldNets.slice().sort();
			var sortedNew = newNets.slice().sort();
			var changed = (sortedOld.length !== sortedNew.length) ||
			sortedOld.some(function(v, i) { return v !== sortedNew[i]; });

			if (changed) {
				uci.set('qemu-vms', name, 'network', newNets); // только если изменился
			}
		} else {
			uci.set('qemu-vms', name, 'network', newNets);
		}

		return uci.save().then(function() {
			ui.hideModal();
			ui.addNotification(null, E('p',
				_('Saved to configuration. Restart the VM for changes to take effect.')), 'info');
			window.location.reload();
		});
	}
});

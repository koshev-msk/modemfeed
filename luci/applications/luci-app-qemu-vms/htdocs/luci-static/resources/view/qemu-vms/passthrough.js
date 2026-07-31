'use strict';
'require view';
'require rpc';
'require ui';
'require dom';
'require uci';

var callListHardware = rpc.declare({
	object: 'luci.qemu-vms',
	method: 'list_hardware',
	params: []
});

var callReleasePci = rpc.declare({
	object: 'luci.qemu-vms',
	method: 'release_pci',
	params: ['section_name']
});

var callReleaseUsb = rpc.declare({
	object: 'luci.qemu-vms',
	method: 'release_usb',
	params: ['section_name']
});

return view.extend({
	load: function() {
		return Promise.all([
			callListHardware(),
			uci.load('qemu-vms')
		]);
	},

	vmNames: function() {
		return uci.sections('qemu-vms', 'vm').map(function(s) { return s['.name']; });
	},

	// --- PCI helpers ---

	findPciSection: function(addr) {
		var found = null;
		uci.sections('qemu-vms', 'pci-passthrough').forEach(function(pt) {
			if (pt.pci_id === addr)
				found = pt['.name'];
		});
		return found;
	},
	/*
	pciOwner: function(addr) {
		var section = this.findPciSection(addr);
		if (!section)
			return null;
		var owner = null;
		uci.sections('qemu-vms', 'vm').forEach(function(vm) {
			if (vm.pci_passthrough === section)
				owner = vm['.name'];
		});
		return owner;
	},
	*/
	pciOwners: function(addr) {
		var section = this.findPciSection(addr);
		if (!section)
			return [];
		var owners = [];
		uci.sections('qemu-vms', 'vm').forEach(function(vm) {
		var list = [].concat(vm.pci_passthrough || []);
		if (list.indexOf(section) !== -1)
			owners.push(vm['.name']);
		});
		return owners;
	},

	// --- USB helpers ---

	findUsbSection: function(vendorId, productId) {
		var found = null;
		uci.sections('qemu-vms', 'usb-passthrough').forEach(function(u) {
			if (u.vendor_id === vendorId && u.product_id === productId)
				found = u['.name'];
		});
		return found;
	},

	usbOwners: function(vendorId, productId) {
		var section = this.findUsbSection(vendorId, productId);
		if (!section)
			return [];
		var owners = [];
		uci.sections('qemu-vms', 'vm').forEach(function(vm) {
			var list = [].concat(vm.usb_passthrough || []);
			if (list.indexOf(section) !== -1)
				owners.push(vm['.name']);
		});
		return owners;
	},

	cleanupVmReferences: function(sectionType, sectionName) {
	// sectionType: 'pci_passthrough' or 'usb_passthrough'
		uci.sections('qemu-vms', 'vm').forEach(function(vm) {
			var list = [].concat(vm[sectionType] || []);
			var index = list.indexOf(sectionName);
			if (index !== -1) {
				list.splice(index, 1);
				if (list.length)
					uci.set('qemu-vms', vm['.name'], sectionType, list);
				 else
					uci.unset('qemu-vms', vm['.name'], sectionType);
				}
			}
		);
	},

	createPciPassthrough: function(dev, ev) {
		var self = this;
		var addr = dev.addr;

		if (this.findPciSection(addr)) {
			ui.addNotification(null, E('p', _('PCI passthrough section already exists for this device.')), 'error');
			return;
		}

		var defaultName = 'pci_' + addr.replace(/[:.]/g, '_');
		var nameInput = E('input', {
			'class': 'cbi-input-text',
			'id': 'new-pci-name',
			'value': defaultName,
			'placeholder': _('e.g. pci_05_00_0')
		});

		ui.showModal(_('Create PCI passthrough for %s').format(addr), [
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Section name')),
				E('div', { 'class': 'cbi-value-field' }, nameInput)
			]),
			E('p', { 'class': 'cbi-section-descr' },
				_('This will bind the device to vfio-pci (if available) and make it available for VM passthrough.')),
			E('div', { 'class': 'right', 'style': 'margin-top: 1em' }, [
				E('button', { 'class': 'btn', 'click': ui.hideModal }, _('Cancel')),
				' ',
				E('button', {
					'class': 'btn cbi-button-positive',
					'click': function() {
						var sectionName = document.getElementById('new-pci-name').value.trim();
						if (!sectionName) {
							ui.addNotification(null, E('p', _('Section name is required')), 'error');
							return;
						}
						if (!/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(sectionName)) {
							ui.addNotification(null, E('p', _('Name must start with a letter or underscore and contain only letters, digits, underscores.')), 'error');
							return;
						}
						if (uci.get('qemu-vms', sectionName)) {
							ui.addNotification(null, E('p', _('A section with this name already exists.')), 'error');
							return;
						}
						self.doCreatePciPassthrough(dev, sectionName);
					}
				}, _('Create'))
			])
		]);
	},

	doCreatePciPassthrough: function(dev, sectionName) {
		var self = this;
		uci.add('qemu-vms', 'pci-passthrough', sectionName);
		uci.set('qemu-vms', sectionName, 'pci_id', dev.addr);
		uci.set('qemu-vms', sectionName, 'vendor_id', dev.vendor_id + ' ' + dev.device_id);

		return uci.save().then(function() {
			ui.hideModal();
			ui.addNotification(null, E('p',
				_('PCI passthrough section "%s" created for %s. Device is now detached from the host driver.').format(sectionName, dev.addr)), 'info');
			self.updateView();
		}).catch(function(err) {
			ui.addNotification(null, E('p', _('Error saving: ') + err.message), 'error');
		});
	},

	deletePciPassthrough: function(addr, ev) {
		var self = this;
		var section = this.findPciSection(addr);
		if (!section) {
			ui.addNotification(null, E('p', _('No passthrough section for this device.')), 'error');
			return;
		}

		var owners = this.pciOwners(addr);
		if (owners.length) {
			ui.addNotification(null, E('p', _('This device is currently attached to VM(s): %s. Please detach it from all VMs first.').format(owners.join(', '))), 'error');
			return;
		}

		ui.showModal(_('Remove PCI passthrough for %s').format(addr), [
			E('p', {}, _('This will remove the passthrough section and re-bind the device to its original host driver (if possible).')),
			E('div', { 'class': 'right', 'style': 'margin-top: 1em' }, [
				E('button', { 'class': 'btn', 'click': ui.hideModal }, _('Cancel')),
				' ',
				E('button', {
					'class': 'btn cbi-button-negative',
					'click': function() {
						return callReleasePci(section).catch(function(err) {
							ui.addNotification(null, E('p',
								_('Warning: could not rebind device to its original driver: ') + err.message), 'warning');
						}).then(function() {
							uci.remove('qemu-vms', section);
							self.cleanupVmReferences('pci_passthrough', section);
							return uci.save();
						}).then(function() {
							ui.hideModal();
							ui.addNotification(null, E('p', _('Passthrough removed for %s.').format(addr)), 'info');
							self.updateView();
						}).catch(function(err) {
							ui.addNotification(null, E('p', _('Error saving: ') + err.message), 'error');
						});
					}
				}, _('Remove'))
			])
		]);
	},

	createUsbPassthrough: function(dev, ev) {
		var self = this;
		var parts = (dev.id || '').split(':');
		var vendorId = parts[0], productId = parts[1];
		if (!vendorId || !productId) {
			ui.addNotification(null, E('p', _('Device has no usable vendor:product ID')), 'error');
			return;
		}

		if (this.findUsbSection(vendorId, productId)) {
			ui.addNotification(null, E('p', _('USB passthrough section already exists for this device.')), 'error');
			return;
		}

		var defaultName = 'usb_' + vendorId + '_' + productId;
		var nameInput = E('input', {
			'class': 'cbi-input-text',
			'id': 'new-usb-name',
			'value': defaultName,
			'placeholder': _('e.g. usb_0bda_c811')
		});

		ui.showModal(_('Create USB passthrough for %s').format(dev.id), [
			E('div', { 'class': 'cbi-value' }, [
				E('label', { 'class': 'cbi-value-title' }, _('Section name')),
				E('div', { 'class': 'cbi-value-field' }, nameInput)
			]),
			E('p', { 'class': 'cbi-section-descr' },
				_('This will make the USB device available for VM passthrough by vendor/product ID.')),
			E('div', { 'class': 'right', 'style': 'margin-top: 1em' }, [
				E('button', { 'class': 'btn', 'click': ui.hideModal }, _('Cancel')),
				' ',
				E('button', {
					'class': 'btn cbi-button-positive',
					'click': function() {
						var sectionName = document.getElementById('new-usb-name').value.trim();
						if (!sectionName) {
							ui.addNotification(null, E('p', _('Section name is required')), 'error');
							return;
						}
						if (!/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(sectionName)) {
							ui.addNotification(null, E('p', _('Name must start with a letter or underscore and contain only letters, digits, underscores.')), 'error');
							return;
						}
						if (uci.get('qemu-vms', sectionName)) {
							ui.addNotification(null, E('p', _('A section with this name already exists.')), 'error');
							return;
						}
						self.doCreateUsbPassthrough(dev, sectionName);
					}
				}, _('Create'))
			])
		]);
	},

	doCreateUsbPassthrough: function(dev, sectionName) {
		var self = this;
		var parts = (dev.id || '').split(':');
		var vendorId = parts[0], productId = parts[1];

		uci.add('qemu-vms', 'usb-passthrough', sectionName);
		uci.set('qemu-vms', sectionName, 'vendor_id', vendorId);
		uci.set('qemu-vms', sectionName, 'product_id', productId);

		return uci.save().then(function() {
			ui.hideModal();
			ui.addNotification(null, E('p',
				_('USB passthrough section "%s" created for %s.').format(sectionName, dev.id)), 'info');
			self.updateView();
		}).catch(function(err) {
			ui.addNotification(null, E('p', _('Error saving: ') + err.message), 'error');
		});
	},

	deleteUsbPassthrough: function(vendorId, productId, ev) {
		var self = this;
		var section = this.findUsbSection(vendorId, productId);
		if (!section) {
			ui.addNotification(null, E('p', _('No passthrough section for this device.')), 'error');
			return;
		}

		var owners = this.usbOwners(vendorId, productId);
		if (owners.length) {
			ui.addNotification(null, E('p', _('This device is currently attached to VM(s): %s. Please detach it from all VMs first.').format(owners.join(', '))), 'error');
			return;
		}

		ui.showModal(_('Remove USB passthrough for %s:%s').format(vendorId, productId), [
			E('p', {}, _('This will remove the passthrough section.')),
			E('div', { 'class': 'right', 'style': 'margin-top: 1em' }, [
				E('button', { 'class': 'btn', 'click': ui.hideModal }, _('Cancel')),
				' ',
				E('button', {
					'class': 'btn cbi-button-negative',
					'click': function() {
						return callReleaseUsb(section).catch(function(err) {
							ui.addNotification(null, E('p',
								_('Warning: could not rebind device to its original driver: ') + err.message), 'warning');
						}).then(function() {
							uci.remove('qemu-vms', section);
							self.cleanupVmReferences('usb_passthrough', section);
							return uci.save();
						}).then(function() {
							ui.hideModal();
							ui.addNotification(null, E('p', _('Passthrough removed.')), 'info');
							self.updateView();
						}).catch(function(err) {
							ui.addNotification(null, E('p', _('Error saving: ') + err.message), 'error');
						});
					}
				}, _('Remove'))
			])
		]);
	},

	renderPciTable: function(devices) {
		var self = this;
		var rows = devices.map(function(dev) {
			var section = self.findPciSection(dev.addr);
			//var owner = self.pciOwner(dev.addr);
			var owners = self.pciOwners(dev.addr);
			var owner = owners.length ? owners.join(', ') : null;
			var passthroughStatus = section ? _('Active<br />Section: %s').format(section) : _('Not passthrough');
			if (owner) passthroughStatus += ' ' + _('<br />Host: %s').format(owner);
			

			var actionButton;
			if (section) {
				actionButton = E('button', {
					'class': 'btn cbi-button-negative btn-sm',
					'click': ui.createHandlerFn(self, 'deletePciPassthrough', dev.addr)
				}, _('Remove passthrough'));
			} else {
				actionButton = E('button', {
					'class': 'btn cbi-button-positive btn-sm',
					'click': ui.createHandlerFn(self, 'createPciPassthrough', dev)
				}, _('Create passthrough'));
			}

			return E('tr', { 'class': 'tr' }, [
				E('td', { 'class': 'td' }, dev.addr),
				E('td', { 'class': 'td' }, dev.vendor_name || '-'),
				E('td', { 'class': 'td' }, dev.device_name || '-'),
				E('td', { 'class': 'td' }, dev.driver || '-'),
				E('td', { 'class': 'td' }, passthroughStatus),
				E('td', { 'class': 'td' }, actionButton)
			]);
		});

		return E('table', { 'class': 'table' }, [
			E('tr', { 'class': 'tr table-titles' }, [
				E('th', { 'class': 'th' }, _('Address')),
				E('th', { 'class': 'th' }, _('Vendor')),
				E('th', { 'class': 'th' }, _('Device')),
				E('th', { 'class': 'th' }, _('Driver')),
				E('th', { 'class': 'th', 'style': 'min-width: 140px;' }, _('Status')),
				E('th', { 'class': 'th' }, _('Action'))
			])
		].concat(rows));
	},

	renderUsbTable: function(devices) {
		var self = this;
		var rows = devices.map(function(dev) {
			var parts = (dev.id || '').split(':');
			var vendorId = parts[0], productId = parts[1];
			var section = (vendorId && productId) ? self.findUsbSection(vendorId, productId) : null;
			var owners = (vendorId && productId) ? self.usbOwners(vendorId, productId) : [];

			var passthroughStatus = section ? _('Active<br />Section: %s').format(section) : _('Not passthrough');
			if (owners.length) passthroughStatus += ' ' + _('<br />Host: %s').format(owners.join(', '));

			var actionButton;
			if (section) {
				actionButton = E('button', {
					'class': 'btn cbi-button-negative btn-sm',
					'click': ui.createHandlerFn(self, 'deleteUsbPassthrough', vendorId, productId)
				}, _('Remove passthrough'));
			} else {
				actionButton = E('button', {
					'class': 'btn cbi-button-positive btn-sm',
					'click': ui.createHandlerFn(self, 'createUsbPassthrough', dev)
				}, _('Create passthrough'));
			}

			return E('tr', { 'class': 'tr' }, [
				E('td', { 'class': 'td' }, dev.bus + ':' + dev.dev),
				E('td', { 'class': 'td' }, dev.id || '-'),
				E('td', { 'class': 'td' }, dev.description || '-'),
				E('td', { 'class': 'td' }, passthroughStatus),
				E('td', { 'class': 'td' }, actionButton)
			]);
		});

		return E('table', { 'class': 'table' }, [
			E('tr', { 'class': 'tr table-titles' }, [
				E('th', { 'class': 'th' }, _('Bus:Dev')),
				E('th', { 'class': 'th' }, _('VID:PID')),
				E('th', { 'class': 'th' }, _('Description')),
				E('th', { 'class': 'th', 'style': 'min-width: 140px;' }, _('Status')),
				E('th', { 'class': 'th' }, _('Action'))
			])
		].concat(rows));
	},

	updateView: function() {
		var self = this;
		if (!this.container) return;

		this.load().then(function(data) {
			var newContent = self.renderContent(data);
			self.container.innerHTML = '';
			self.container.appendChild(newContent);
		}).catch(function(err) {
			ui.addNotification(null, E('p', _('Failed to refresh view: ') + err.message), 'error');
		});
	},

	render: function(data) {
		var hw = data[0];
		var container = E('div', {});

		var content = this.renderContent(data);
		container.appendChild(content);

		this.container = container;

		return container;
	},

	renderContent: function(data) {
		var hw = data[0];
		var self = this;
	
		var description = E('div', { 'class': 'cbi-section' }, [
			E('h2', {}, _('Hardware Passthrough')),
				E('p', { 'class': 'cbi-section-descr' },
				_('This page allows you to configure passthrough of PCI and USB devices to virtual machines. ') +
				'<br />' + _('Changes take effect after VM restart.'))
		]);

		// tabs
		var tabs = E('ul', { 'class': 'cbi-tabmenu' }, [
			E('li', { 'class': 'cbi-tab', 'data-tab': 'pci' }, [
				E('a', {
					'href': '#',
					'click': function(ev) {
						ev.preventDefault();
						self.switchTab('pci');
					}
				}, _('PCI devices'))
			]),
			E('li', { 'class': 'cbi-tab-disabled', 'data-tab': 'usb' }, [
				E('a', {
					'href': '#',
					'click': function(ev) {
						ev.preventDefault();
						self.switchTab('usb');
					}
				}, _('USB devices'))
			])
		]);

		var pciPanel = E('div', { 'class': 'cbi-tabpanel', 'id': 'pci-panel' }, [
			E('p', { 'class': 'cbi-section-descr' },
				_('PCI passthrough requires IOMMU enabled in the host BIOS and kernel command line (intel_iommu=on / amd_iommu=on), and the vfio-pci kernel module loaded.') +
				'<br />' + _('Creating a passthrough section will bind the device to vfio-pci (if possible). Removing it will attempt to re-bind to the original driver.') +
				'<br />' + _('Note: Some devices (especially Intel network cards) may require the vfio-pci module option "disable_idle_d3=1" to prevent them from entering a low-power state.') +
				'<br />' + _('Change to "vfio-pci disable_idle_d3=1" in /etc/modules.d/vfio-pci if you encounter "No such device" errors.')),
			this.renderPciTable(hw.pci || [])
		]);

		var usbPanel = E('div', { 'class': 'cbi-tabpanel', 'id': 'usb-panel', 'style': 'display:none;' }, [
			E('p', { 'class': 'cbi-section-descr' },
				_('USB passthrough is based on vendor/product ID. The device will be available to any VM that includes the corresponding USB passthrough section.')),
			this.renderUsbTable(hw.usb || [])
		]);

		return E('div', {}, [description, tabs, pciPanel, usbPanel]);
	},

	switchTab: function(tab) {
		// all tabs
		var tabs = document.querySelectorAll('li.cbi-tab, li.cbi-tab-disabled');
		tabs.forEach(function(el) {
			if (el.dataset.tab === tab) {
				el.className = 'cbi-tab';
			} else {
				el.className = 'cbi-tab-disabled';
			}
		});

		var pciPanel = document.getElementById('pci-panel');
		var usbPanel = document.getElementById('usb-panel');
		if (pciPanel) pciPanel.style.display = (tab === 'pci') ? '' : 'none';
		if (usbPanel) usbPanel.style.display = (tab === 'usb') ? '' : 'none';
	},
});

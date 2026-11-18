'use strict';
'require view';
'require ui';
'require wizzard.wizard as wizard';

return view.extend({
	load: function() {
		return wizard.getFlags();
	},

	render: function(flags) {
		var auto = E('input', {
			type: 'checkbox',
			change: function(ev) {
				L.resolveDefault(wizard.setFlags({ hide: ev.target.checked ? '0' : '1' }), null);
			}
		});

		auto.checked = !flags.hide;

		return E([
			E('h2', _('Setup Wizard')),
			E('div', { 'class': 'cbi-section-descr' },
				_('Step-by-step first setup: router name and time, internet connection, Wi-Fi and administrator password.')),
			E('p', {}, flags.done ? _('The wizard has been completed on this router.') :
				_('The wizard has not been completed yet.')),
			E('p', {}, E('label', {}, [ auto, ' ', _('Open the wizard automatically on the Status page') ])),
			E('p', {}, E('button', {
				'class': 'cbi-button cbi-button-action important',
				click: function() { wizard.open(); }
			}, _('Start wizard')))
		]);
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});

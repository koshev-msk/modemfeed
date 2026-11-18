'use strict';
'require baseclass';
'require wizzard.wizard as wizard';

/*
 * Runs once per Status page load. The section stays hidden
 * (render() is not defined), only oneshot() opens the wizard modal
 * when the wizard was neither completed nor switched off.
 */
return baseclass.extend({
	title: '',

	load: function() {
		if (this.checked)
			return Promise.resolve(false);

		this.checked = true;

		return wizard.shouldAutostart();
	},

	oneshot: function(autostart) {
		if (autostart)
			wizard.open();
	}
});

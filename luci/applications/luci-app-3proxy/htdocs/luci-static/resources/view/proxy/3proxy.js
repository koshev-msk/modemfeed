'use strict';
'require fs';
'require view';
'require ui';
'require uci';


return view.extend({
	load: function() {
		return uci.load('3proxy');
	},

	render: function() {
		var configPath = '/etc/3proxy.cfg';
		
		uci.sections('3proxy', '3proxy').forEach(function(s) {
			if (s['.type'] === '3proxy' && s.config) {
				configPath = s.config;
			}
		});
		
		this.configPath = configPath;
		
		var container = E('div', { 'class': 'cbi-map' }, [
			E('h2', {}, _('3proxy Configuration')),
			E('div', { 'class': 'cbi-section' }, [
				E('div', { 'class': 'cbi-section-descr' }, [
					_('Edit 3proxy configuration file.'),
					E('br'),
					_('Config file: '),
					E('code', {}, configPath)
				]),
				E('div', { 'class': 'cbi-value' }, [
					E('label', { 
						'class': 'cbi-value-title',
						'style': 'display: block; font-weight: bold;' 
					}, _('Configuration')),
					E('br'), E('br'),
					E('textarea', {
						'class': 'cbi-input-textarea',
						'rows': 20,
						'style': 'width: 100%; box-sizing: border-box;',
						'name': 'config',
						'id': 'config-textarea'
					}, '')
				]),
				E('div', { 
					'class': 'cbi-value',
					'style': 'display: flex; justify-content: flex-end; margin-top: 20px; gap: 10px;' 
				}, [
					E('div', { 'class': 'cbi-value-field right' }, [
						E('button', {
							'class': 'cbi-button cbi-button-save',
							'click': ui.createHandlerFn(this, 'saveConfig')
						}, _('Save')),
						' ',
						E('button', {
							'class': 'cbi-button cbi-button-apply',
							'click': ui.createHandlerFn(this, 'restartService')
						}, _('Restart 3proxy'))
					])
				])
			])
		]);
		
		var self = this;
		fs.read(configPath).catch(function(err) {
			return '';
		}).then(function(content) {
			var textarea = document.getElementById('config-textarea');
			if (textarea) {
				textarea.value = content || '';
			}
		});
		
		return container;
	},

	saveConfig: function() {
		var textarea = document.querySelector('textarea[name="config"]');
		if (!textarea) {
			textarea = document.getElementById('config-textarea');
		}
		
		var config = textarea.value.trim().replace(/\r\n/g, '\n') + '\n';
		
		var configPath = this.configPath || '/etc/3proxy.cfg';

		return fs.write(configPath, config).then(function() {
			ui.addNotification(null, E('p', _('Configuration saved successfully')), 'info');
		}).catch(function(err) {
			ui.addNotification(null, E('p', _('Error saving configuration: ') + err.message), 'error');
		});
	},

	restartService: function() {
		return fs.exec('/etc/init.d/3proxy', ['restart']).then(function(res) {
			if (res.code === 0) {
				ui.addNotification(null, E('p', _('3proxy restarted successfully')), 'info');
			} else {
				ui.addNotification(null, E('p', _('Error restarting 3proxy: ') + res.stderr), 'error');
			}
		}).catch(function(err) {
			ui.addNotification(null, E('p', _('Error restarting 3proxy: ') + err.message), 'error');
		});
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});

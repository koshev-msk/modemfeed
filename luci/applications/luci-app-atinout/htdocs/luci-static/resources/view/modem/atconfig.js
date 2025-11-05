'use strict';
'require fs';
'require view';
'require ui';
'require rpc';

/*
        Copyright 2022-2023 Rafał Wabik - IceG - From eko.one.pl forum

        Modified for atinout by Konstantine Shevlakov <shevlakov@132lan.ru> 2023

        Licensed to the GNU General Public License v3.0.
*/

var cmddesc = _("Each line must have the following format: <code>AT command description;AT+COMMAND</code>.<br />For user convenience, the file is saved to the location <code>/etc/atcommands.user</code>.");

return view.extend({
        load: function() {
                return fs.read('/etc/atcommands.user').catch(function(err) {
                        return '';
                });
        },

        render: function(content) {
                var content = content || '';

                return E('div', { 'class': 'cbi-map' }, [
                        E('div', { 'class': 'cbi-map-descr' }, _('AT Commands Configuration')),
                        E('div', { 'class': 'cbi-section' }, [
                                E('div', { 'class': 'cbi-section-descr' }, cmddesc),
                                E('div', { 'class': 'cbi-value' }, [
                                        E('label', { 'class': 'cbi-value-title' }, _('User AT commands')),
                                        E('div', { 'class': 'cbi-value-field' }, [
                                                E('textarea', {
                                                        'class': 'cbi-input-textarea',
                                                        'rows': 20,
                                                        'style': 'width: 100%',
                                                        'name': 'atcommands'
                                                }, content)
                                        ])
                                ]),
                                E('div', { 
                                        'class': 'cbi-value',
                                        'style': 'display: flex; justify-content: flex-end; margin-top: 20px;' 
                                }, [
                                        E('div', { 'class': 'cbi-value-field right' }, [
                                                E('button', {
                                                        'class': 'cbi-button cbi-button-save',
                                                        'click': ui.createHandlerFn(this, 'saveCommands')
                                                }, _('Save'))
                                        ])
                                ])
                        ])
                ]);
        },

        saveCommands: function(ev) {
                var textarea = document.querySelector('textarea[name="atcommands"]');
                var commands = textarea.value.trim().replace(/\r\n/g, '\n') + '\n';

                return fs.write('/etc/atcommands.user', commands).then(function() {
                        ui.addNotification(null, E('p', _('AT commands list saved successfully')), 'info');
                }).catch(function(err) {
                        ui.addNotification(null, E('p', _('Error saving list: ') + err.message), 'error');
                });
        },

        handleSaveApply: null,
        handleSave: null,
        handleReset: null
});

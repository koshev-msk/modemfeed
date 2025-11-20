'use strict';
'require dom';
'require form';
'require fs';
'require ui';
'require uci';
'require view';

/*
        Copyright 2025 Konstantine Shevlakov <shevlakov@132lan.ru> 
        Licensed to the GNU General Public License v3.0.
*/

return view.extend({

    load: function() {
        return L.resolveDefault(fs.exec_direct('/usr/bin/msg_control', [ 'sent' ]));
    },

    handleClear: function(ev) {
        return L.resolveDefault(fs.exec_direct('/usr/bin/msg_control', [ 'rmsent' ])).then(function() {
            location.reload();
        });
    },

    handleDelete: function(filename) {
        if (confirm(_('Are you sure you want to delete this message?'))) {
            return L.resolveDefault(fs.exec_direct('/usr/bin/msg_control', [ 'delete', filename ])).then(function() {
                location.reload();
            });
        }
    },

    handleRefresh: function(ev) {
        location.reload();
    },

    render: function (data) {
        var obj = JSON.parse(data);
        let tableHeaders = [
            _('Modem'),
            _('Send Date'),
            _('Time(sec)'),
            _('To'),
            _('Message'),
            _('Action')
        ];

        let tableSMS = E('table', { 'class': 'table' },
            E('tr', { 'class': 'tr cbi-section-table-titles' }, [
                E('th', { 'class': 'th left', 'width': '12%' }, tableHeaders[0]),
                E('th', { 'class': 'th left', 'width': '12%' }, tableHeaders[1]),
                E('th', { 'class': 'th left', 'width': '12%' }, tableHeaders[2]),
                E('th', { 'class': 'th left', 'width': '18%' }, tableHeaders[3]),
                E('th', { 'class': 'th left', 'width': '41%' }, tableHeaders[4]),
                E('th', { 'class': 'th left', 'width': '5%' }, tableHeaders[5]),
            ]),
        );

        var s = 1;
        for (let i = 0; i < obj.sent.length; i++) {
            let message = obj.sent[i];

            if (!message.filename) {
                continue;
            }

            let to = message.to;
            if (to && to.length > 6 && Number(to)) {
                to = '+' + to;
            } else {
                to = to || '';
            }

            let content = message.content || _('No content');

            tableSMS.append(
                E('tr', { 'class': 'cbi-rowstyle-'+s }, [
                    E('td', { 'class': 'td left', 'data-title': tableHeaders[0] }, message.modem || ''),
                    E('td', { 'class': 'td left', 'data-title': tableHeaders[1] }, message.sent || ''),
                    E('td', { 'class': 'td left', 'data-title': tableHeaders[2] }, message.time || ''),
                    E('td', { 'class': 'td left', 'data-title': tableHeaders[3] }, to),
                    E('td', { 'class': 'td left', 'data-title': tableHeaders[4] }, E('div', { 
                        'style': 'max-height: 100px; overflow-y: auto; word-wrap: break-word;'
                    }, content)),
                    E('td', { 'class': 'td left', 'data-title': tableHeaders[5] }, 
                        E('button', {
                            'class': 'cbi-button cbi-button-remove',
                            'click': ui.createHandlerFn(this, 'handleDelete', message.filename),
                            'title': _('Delete this message')
                        }, [ '×' ])
                    )
                ]),
            );
            s = (s % 2) + 1;
        };

        var button = ( 
            E('hr'),
                E('div', { 'class': 'right'  }, [
                E('button', { 
                    'class': 'cbi-button cbi-button-remove', 
                    'click': ui.createHandlerFn(this, 'handleClear')
                }, [ _('Remove All SMS') ]),
                '\xa0\xa0\xa0',
                E('button', {
                    'class': 'cbi-button cbi-button-save', 
                    'click': ui.createHandlerFn(this, 'handleRefresh')
                }, [ _('Refresh') ])
            ])
        );

        var result = E('fieldset', { 'class': 'cbi-section' }, [
            E('h2', {}, _('Smstools3: Outgoing messages')), 
            tableSMS, 
            button
        ]);
        return result;
    },
    handleSaveApply: null,    
    handleSave: null,
    handleReset: null
});

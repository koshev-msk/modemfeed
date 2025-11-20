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
		L.resolveDefault(fs.exec_direct('/usr/share/luci-app-smstools3/led.sh', [ 'off' ]));
		return L.resolveDefault(fs.exec_direct('/usr/bin/msg_control', [ 'recv' ]));
	},

	handleClear: function(ev) {
		return L.resolveDefault(fs.exec_direct('/usr/bin/msg_control', [ 'rmrecv' ])).then(function() {
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
			_('Recv.Date'),
			_('From'),
			_('Message'),
			_('Action')
		];

		let tableSMS = E('table', { 'class': 'table' },
			 E('tr', { 'class': 'tr table-titles' }, [
				E('th', { 'class': 'th left', 'width': '10%' }, tableHeaders[0]),
				E('th', { 'class': 'th left', 'width': '10%' }, tableHeaders[1]),
				E('th', { 'class': 'th left', 'width': '10%' }, tableHeaders[2]),
				E('th', { 'class': 'th left', 'width': '20%' }, tableHeaders[3]),
				E('th', { 'class': 'th left', 'width': '45%' }, tableHeaders[4]),
				E('th', { 'class': 'th left', 'width': '5%' }, tableHeaders[5]),
			]),
		);

		var s = 1;
		for (let i = 0; i < obj.recv.length; i++) {
			if (obj.recv[i].from.length > 6 && Number(obj.recv[i].from)) {
				var from = '+' + obj.recv[i].from;
			} else {
				var from = obj.recv[i].from;
			}
			//  get msg filename
			var filename = obj.recv[i].filename || '';

			tableSMS.append(E('tr', { 'class': 'tr cbi-rowstyle-'+s }, [
					E('td', { 'class': 'td left', 'data-title': tableHeaders[0], 'width': '10%' }, obj.recv[i].modem),
					E('td', { 'class': 'td left', 'data-title': tableHeaders[1], 'width': '10%' }, obj.recv[i].srecv),
					E('td', { 'class': 'td left', 'data-title': tableHeaders[2], 'width': '10%' }, obj.recv[i].drecv),
					E('td', { 'class': 'td left', 'data-title': tableHeaders[3], 'width': '20%' }, from),
					E('td', { 'class': 'td left', 'data-title': tableHeaders[4], 'width': '45%' }, obj.recv[i].content),
					E('td', { 'class': 'td left', 'data-title': tableHeaders[5], 'width': '5%' }, 
						E('button', {
							'class': 'cbi-button cbi-button-remove',
							'click': ui.createHandlerFn(this, 'handleDelete', obj.recv[i].filename),
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
					'class': 'cbi-button cbi-button-remove', 'id': 'clr', 'click': ui.createHandlerFn(this, 'handleClear')
				}, [ _('Remove All SMS') ]),
				'\xa0\xa0\xa0',
				E('button', {
					'class': 'cbi-button cbi-button-save', 'id': 'clr', 'click': ui.createHandlerFn(this, 'handleRefresh')
				}, [ _('Refresh') ])
			])
		);
		var result = E('fieldset', { 'class': 'cbi-section' }, [E('h2', {}, _('Smstools3: Incoming messages')), tableSMS, button]);
		return result;
	},
	handleSaveApply: null,	
	handleSave: null,
	handleReset:  null
});

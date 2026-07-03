'use strict';
'require poll';
'require view';
'require rpc';

const callMwan3Status = rpc.declare({
	object: 'mwan3',
	method: 'status',
	params: ['section'],
	expect: {  },
});

function getStatusBackgroundClass(status) {
	switch (status) {
		case 'online':
			return 'label success';
		case 'offline':
			return 'label warning';
                case 'disabled':
			return 'label';
		case 'notracking':
			return 'label';
		default:
			return 'label';
	}
}

function renderMwan3Status(status) {
	if (!status.interfaces)
		return '<strong>%h</strong>'.format(_('No MWAN interfaces found'));

	var tableHtml = '<table class="table cbi-section-table" style="width: 100%;">';
	tableHtml += '<thead><tr class="tr cbi-section-table-titles">';
	tableHtml += '<th class="th left">' + _('Interface') + '</th>';
	tableHtml += '<th class="th left">' + _('Status') + '</th>';
	tableHtml += '<th class="th left">' + _('Uptime') + '</th>';
	tableHtml += '</tr></thead><tbody>';

	for (var iface in status.interfaces) {
		var state = '';
		var time = '';
		var statusType = status.interfaces[iface].status;
		
		switch (statusType) {
			case 'online':
				state = _('Online');
				time = '%t'.format(status.interfaces[iface].online);
				break;
			case 'offline':
				state = _('Offline');
				time = '%t'.format(status.interfaces[iface].offline);
				break;
			case 'notracking':
				state = _('No Tracking');
				if ((status.interfaces[iface].uptime) > 0) {
					time = '%t'.format(status.interfaces[iface].uptime);
				} else {
					time = '';
				}
				break;
			default:
				state = _('Disabled');
				time = '';
				break;
		}
		
		var statusClass = getStatusBackgroundClass(statusType);
		
		tableHtml += '<tr class="tr cbi-section-table-row">';
		tableHtml += '<td class="td left"><strong>' + iface + '</strong></td>';
		tableHtml += '<td class="td left"><span class="' + statusClass + '">' + state + '</span></td>';
		tableHtml += '<td class="td left">' + (time || '') + '</td>';
		tableHtml += '</tr>';
	}
	
	tableHtml += '</tbody></table>';
	
	return tableHtml;
}

return view.extend({
	load: function() {
		return Promise.all([
			callMwan3Status("interfaces"),
		]);
	},

	render: function (data) {
		poll.add(function() {
			return callMwan3Status("interfaces").then(function(result) {
				var view = document.getElementById('mwan3-service-status');
				view.innerHTML = renderMwan3Status(result);
			});
		});

		return E('div', { class: 'cbi-map' }, [
			E('h2', [ _('MultiWAN Manager - Overview') ]),
			E('div', { class: 'cbi-section' }, [
				E('div', { 'id': 'mwan3-service-status' }, [
					E('em', { 'class': 'spinning' }, [ _('Collecting data ...') ])
				])
			])
		]);
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});

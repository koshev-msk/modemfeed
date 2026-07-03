'use strict';
'require baseclass';
'require rpc';
'require uci';

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

return baseclass.extend({
	title: _('MultiWAN Manager'),

	load: function() {
		return uci.load('mwan3').then(function() {
			var globals = uci.sections('mwan3', 'globals');
			if (!globals || !globals[0] || globals[0].show_overview !== '1')
				return null;
			return Promise.all([
				callMwan3Status("interfaces"),
			]);
		});
	},

	render: function (result) {
		if (!result)
			return null;

		if (!result[0]?.interfaces || Object.keys(result[0].interfaces).length === 0)
			return null;

		// Основной контейнер
		var container = E('div', { 'class': 'cbi-section' });

		// Таблица с системными классами LuCI
		var table = E('table', { 'class': 'table cbi-section-table' });

		// Заголовок
		var thead = E('tr', { 'class': 'tr cbi-section-table-titles' });
		thead.appendChild(E('th', { 'class': 'th left' }, [ _('Interface') ]));
		thead.appendChild(E('th', { 'class': 'th left' }, [ _('Status') ]));
		thead.appendChild(E('th', { 'class': 'th left' }, [ _('Uptime') ]));
		table.appendChild(thead);

		// Тело таблицы
		var tbody = E('tbody');

		for (var iface in result[0].interfaces) {
			var state = '';
			var time = '';
			var statusType = result[0].interfaces[iface].status;

			switch (statusType) {
				case 'online':
					state = _('Online');
					time = '%t'.format(result[0].interfaces[iface].online);
					break;
				case 'offline':
					state = _('Offline');
					time = '%t'.format(result[0].interfaces[iface].offline);
					break;
				case 'notracking':
					state = _('No Tracking');
					if ((result[0].interfaces[iface].uptime) > 0) {
						time = '%t'.format(result[0].interfaces[iface].uptime);
					} else {
						time = '';
					}
					break;
				default:
					state = _('Disabled');
					time = '';
					break;
			}

			var tr = E('tr', { 'class': 'tr cbi-section-table-row' });

			// Ячейка интерфейса
			tr.appendChild(E('td', { 'class': 'td left' }, [
				E('strong', {}, [ iface ])
			]));

			// Ячейка статуса с цветным фоном через системные классы label
			var statusClass = getStatusBackgroundClass(statusType);
			tr.appendChild(E('td', { 'class': 'td left' }, [
				E('span', { 'class': statusClass }, [ state ])
			]));

			// Ячейка времени
			tr.appendChild(E('td', { 'class': 'td left' }, [ time || '' ]));

			tbody.appendChild(tr);
		}

		table.appendChild(tbody);
		container.appendChild(table);

		return container;
	}
});

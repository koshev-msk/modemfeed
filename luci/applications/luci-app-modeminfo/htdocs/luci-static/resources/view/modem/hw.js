'use strict';
'require baseclass';
'require form';
'require fs';
'require view';
'require ui';
'require uci';
'require poll';
'require dom';
'require tools.widgets as widgets';

/*
	Copyright Konstantine Shevlakov <shevlakov@132lan.ru> 2023-2026

	Licensed to the GNU General Public License v3.0.

	Refactored: bug fixes, XSS prevention, deduplication.
*/

// ─── Helpers ──────────────────────────────────────────────────────────────────

/**
 * Safe getElementById.
 * @param {string} id
 * @returns {HTMLElement|null}
 */
function getEl(id) {
	return document.getElementById(id);
}

/**
 * Write a value into a cell by id.
 * Falls back to '--' when value is missing/placeholder.
 * Uses textContent to prevent XSS.
 *
 * @param {string} id    - Element id
 * @param {string} value - Raw value from modem JSON
 * @param {string} [suffix] - Optional suffix appended to a real value (e.g. ' °C')
 */
function setCell(id, value, suffix) {
	const el = getEl(id);
	if (!el) return;
	const missing = !value || value === '--';
	el.textContent = missing ? '--' : value + (suffix || '');
}

// ─── Field definitions ────────────────────────────────────────────────────────
// Each entry: { key: modem JSON field, suffix: optional unit string }
const MODEM_FIELDS = [
	{ key: 'device',   suffix: ''    },
	{ key: 'firmware', suffix: ''    },
	{ key: 'imsi',     suffix: ''    },
	{ key: 'iccid',    suffix: ''    },
	{ key: 'imei',     suffix: ''    },
	{ key: 'chiptemp', suffix: ' °C' },
];

// ─── Main view ────────────────────────────────────────────────────────────────

return view.extend({

	// FIX: removed unused `data` parameter
	load: function() {
		return L.resolveDefault(fs.exec_direct('/usr/bin/modeminfo'), '{"modem": []}');
	},

	polldata: poll.add(function() {
		return L.resolveDefault(fs.exec_direct('/usr/bin/modeminfo'), '{"modem": []}')
			.then(function(res) {
				// FIX: wrapped in try/catch — was crashing on invalid JSON
				let json;
				try {
					json = JSON.parse(res);
				} catch (e) {
					console.error('modeminfo hw: JSON parse error', e);
					return;
				}

				if (!json || !Array.isArray(json.modem)) return;

				for (let i = 0; i < json.modem.length; i++) {
					const modem = json.modem[i];

					// FIX: replaced 6 copy-pasted blocks with a single loop.
					// FIX: `var view` renamed — was shadowing the LuCI `view` module.
					// FIX: value '--' now explicitly written to cell (was silently ignored).
					// FIX: innerHTML → textContent to prevent XSS.
					// FIX: String.format(x) with no extra args replaced by direct assignment.
					for (const { key, suffix } of MODEM_FIELDS) {
						setCell(key + i, modem[key], suffix);
					}
				}
			});
	}),

	render: function(data) {
		// FIX: wrapped in try/catch — was crashing on invalid JSON
		let json;
		try {
			json = JSON.parse(data);
		} catch (e) {
			json = { modem: [] };
		}

		const m = new form.Map('modeminfo', _('Modeminfo: Hardware'), _('Hardware and sim-card info.'));
		const s = m.section(form.TypedSection, 'general', null);
		s.anonymous = true;

		for (let i = 0; i < json.modem.length; i++) {
			const idx = i + 1;

			// Pre-build all element ids for this modem slot
			const ids = {};
			for (const { key } of MODEM_FIELDS) {
				ids[key] = key + i;
			}

			let o;
			if (json.modem.length > 1) {
				s.tab('modem' + i, _('Modem') + ' ' + idx);
				o = s.taboption('modem' + i, form.HiddenValue, 'generic');
			} else {
				o = s.option(form.HiddenValue, 'generic');
			}

			o.render = L.bind(function() {
				return E('div', {}, [
					E('h3', { 'class': 'data-tab' }),
					E('div', { 'class': 'cbi-section' }, [
						E('table', { 'class': 'table' }, [
							E('tr', { 'class': 'tr cbi-rowstyle-2' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('Device')]),
								E('td', { 'class': 'td left', 'id': ids.device }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-1' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('Firmware')]),
								E('td', { 'class': 'td left', 'id': ids.firmware }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-2' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('IMSI')]),
								E('td', { 'class': 'td left', 'id': ids.imsi }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-1' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('ICCID')]),
								E('td', { 'class': 'td left', 'id': ids.iccid }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-2' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('IMEI')]),
								E('td', { 'class': 'td left', 'id': ids.imei }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-1' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('Chiptemp')]),
								E('td', { 'class': 'td left', 'id': ids.chiptemp }, ['--']),
							]),
						])
					])
				]);
			}, this.polldata);

			o.anonymous = true;
			o.rmempty   = true;
		}

		return m.render();
	},

	handleSaveApply: null,
	handleSave:      null,
	handleReset:     null,
});

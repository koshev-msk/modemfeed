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
	Copyright Konstantine Shevlakov <shevlakov@132lan.ru> 2023

	Licensed to the GNU General Public License v3.0.

	Refactored: bug fixes, performance improvements, code deduplication.
*/

const REGISTERED_STATUSES = [1, 6, 9];
const ROAMING_STATUSES    = [3, 5, 7, 10];

// Signal Icons
const SIGNAL_ICONS = [
	{ max: 10,       icn: 'signal-000-000.svg' },
	{ max: 25,       icn: 'signal-000-025.svg' },
	{ max: 50,       icn: 'signal-025-050.svg' },
	{ max: 75,       icn: 'signal-050-075.svg' },
	{ max: Infinity, icn: 'signal-075-100.svg' },
];

// Registration Status Labels
const REG_STATUSES = new Map([
	[0, _('No Registration')],
	[2, _('Searching')],  [8, _('Searching')],
	[3, _('Denied')],
	[4, _('Unknown')],
	[5, _('Roaming')],    [7, _('Roaming')], [10, _('Roaming')],
]);

// Progress bar configuration
// calc(vn, mn): vn = clamped signal value, mn = min boundary
// All formulas normalise the value to 0–100% range for the progress bar.
const PROGRESS_CONFIG = {
	rssi: {
		selector: '#rssi',
		min: -110, max: -50,
		// Linear map from [-110..-50] → [0..100]
		calc: (vn, mn) => Math.floor(100 * (1 - (-50 - vn) / (-50 - mn))),
	},
	rsrp: {
		selector: '#rsrp',
		min: -140, max: -50,
		// Slightly expanded scale (×1.2) to better use bar width
		calc: (vn, mn) => Math.floor(120 * (1 - (-50 - vn) / (-70 - mn))),
	},
	sinr: {
		selector: '#sinr',
		min: -20, max: 30,
		// Linear map from [-20..30] → [0..100]
		calc: (vn, mn) => Math.floor(100 - (100 * (1 - ((mn - vn) / (mn - 30))))),
	},
	rsrq: {
		selector: '#rsrq',
		min: -20, max: 0,
		// Proportional map from [-20..0] → [0..100]
		calc: (vn, mn) => Math.floor(115 - (100 / mn) * vn),
	},
	ecio: {
		selector: '#sinr',
		min: -24, max: 0,
		// Same shape as rsrq but for EC/IO range [-24..0]
		calc: (vn, mn) => Math.floor(100 - (100 / mn) * vn),
	},
};

// LTE EARFCN band/frequency table
const LTE_BANDS = [
	{ min: 0,     max: 599,   frdl: 2110,  frul: 1920,  offset: 0,     band: '1'  },
	{ min: 600,   max: 1199,  frdl: 1930,  frul: 1850,  offset: 600,   band: '2'  },
	{ min: 1200,  max: 1949,  frdl: 1805,  frul: 1710,  offset: 1200,  band: '3'  },
	{ min: 1950,  max: 2399,  frdl: 2110,  frul: 1710,  offset: 1950,  band: '4'  },
	{ min: 2400,  max: 2469,  frdl: 869,   frul: 824,   offset: 2400,  band: '5'  },
	{ min: 2750,  max: 3449,  frdl: 2620,  frul: 2500,  offset: 2750,  band: '7'  },
	{ min: 3450,  max: 3799,  frdl: 925,   frul: 880,   offset: 3450,  band: '8'  },
	{ min: 6150,  max: 6449,  frdl: 791,   frul: 832,   offset: 6150,  band: '20' },
	{ min: 9210,  max: 9659,  frdl: 758,   frul: 703,   offset: 9210,  band: '28' },
	{ min: 9870,  max: 9919,  frdl: 452.5, frul: 462.5, offset: 9870,  band: '31' },
	{ min: 37750, max: 38249, frdl: 2570,  frul: 2570,  offset: 37750, band: '38' },
	{ min: 38650, max: 39649, frdl: 2300,  frul: 2300,  offset: 38650, band: '40' },
	{ min: 39650, max: 41589, frdl: 2496,  frul: 2496,  offset: 39650, band: '41' },
];

// Non-LTE (UMTS/GSM) ARFCN band/frequency table
const NON_LTE_BANDS = [
	{
		condition: rfcn => rfcn >= 10562 && rfcn <= 10838,
		calc:      rfcn => ({ offset: 950, dlfreq: rfcn / 5,                        ulfreq: (rfcn - 950) / 5,          band: 'IMT2100' }),
	},
	{
		condition: rfcn => rfcn >= 2937  && rfcn <= 3088,
		calc:      rfcn => ({ frul: 925,  ulfreq: 340 + (rfcn / 5),                 dlfreq: (340 + (rfcn / 5)) - 45,   band: 'UMTS900' }),
	},
	{
		condition: rfcn => rfcn >= 955   && rfcn <= 1023,
		calc:      rfcn => ({ frul: 890,  ulfreq: 890 + ((rfcn - 1024) / 5),        dlfreq: (890 + ((rfcn - 1024) / 5)) + 45, band: 'DSC900' }),
	},
	{
		condition: rfcn => rfcn >= 512   && rfcn <= 885,
		calc:      rfcn => ({ frul: 1710, ulfreq: 1710 + ((rfcn - 512) / 5),        dlfreq: (1710 + ((rfcn - 512) / 5)) + 95, band: 'DCS1800' }),
	},
	{
		condition: rfcn => rfcn >= 1     && rfcn <= 124,
		calc:      rfcn => ({ frul: 890,  ulfreq: 890 + (rfcn / 5),                 dlfreq: (890 + (rfcn / 5)) + 45,   band: 'GSM900' }),
	},
];

const UMTS_MODES = /(HS|3G|UMTS|WCDMA)/i;

// Helpers

/**
 * Safe getElementById wrapper.
 * @param {string} id
 * @returns {HTMLElement|null}
 */
function getEl(id) {
	return document.getElementById(id);
}

/**
 * Show or hide the grandparent row of a signal element.
 * @param {HTMLElement} el
 * @param {boolean} show
 */
function setRowVisible(el, show) {
	const row = el && el.parentElement && el.parentElement.parentElement;
	if (row) row.style.display = show ? '' : 'none';
}

/**
 * Update a cbi-progressbar element.
 * @param {string}  type   - Key in PROGRESS_CONFIG
 * @param {string}  value  - Raw signal value string (e.g. "-85 dBm")
 * @param {number}  max    - Boundary value (passed as second calc argument)
 * @param {number}  idx    - Modem index suffix for the element id
 */
function updateProgressBar(type, value, max, idx) {
	const config = PROGRESS_CONFIG[type];
	if (!config) return;

	const pg = document.querySelector(`${config.selector}${idx}`);
	if (!pg) return;

	const vn = Math.max(config.min, Math.min(config.max, parseInt(value) || 0));
	const mn = parseInt(max) || 100;
	const pc = Math.min(100, Math.max(0, config.calc(vn, mn)));

	pg.firstElementChild.style.width              = `${pc}%`;
	pg.firstElementChild.style.animationDirection = 'reverse';
	pg.setAttribute('title', String(value));
}

/**
 * Format distance string, returns empty string when unavailable.
 * @param {string|number} dist
 * @returns {string}
 */
function formatDistance(dist) {
	if (!dist || dist === '--' || dist === '' || dist === '0.00') return '';
	return ' ~' + dist + ' km';
}

/**
 * Build the operator status badge HTML safely using LuCI E() helper.
 * @param {Object} modem  - Single modem entry from JSON
 * @param {string} icon   - URL to signal icon
 * @param {string} reg    - Human-readable registration status
 * @returns {string}      - Safe innerHTML string via dom.content
 */
function formatModemStatus(modem, icon, reg) {
	const rg           = parseInt(modem.reg) || 0;
	const p            = modem.csq_per || 0;
	const cops         = modem.cops    || '--';
	const color        = modem.csq_col || '#000000';
	const distanceText = formatDistance(modem.distance);

	const iconEl  = icon ? E('img', { 'class': 'modem-signal-icon', 'src': icon }) : null;
	const boldEl  = E('b', { 'style': `color:${color}` }, [`${p}%`]);

	const children = iconEl
		? [cops + ' ', iconEl, ' ', boldEl, distanceText]
		: [cops + ' ', boldEl, distanceText];

	if (REGISTERED_STATUSES.includes(rg)) {
		return E('span', { 'class': 'ifacebadge' }, children).outerHTML;
	} else if (ROAMING_STATUSES.includes(rg)) {
		return E('span', { 'class': 'ifacebadge' }, [`${cops} (${reg}) `, ...(iconEl ? [iconEl, ' '] : []), boldEl, distanceText]).outerHTML;
	} else {
		return E('span', { 'class': 'ifacebadge' }, [reg || '--']).outerHTML;
	}
}

/**
 * Derive signal icon URL from signal percentage.
 * @param {number} pct
 * @returns {string}
 */
function resolveSignalIcon(pct) {
	const { icn } = SIGNAL_ICONS.find(({ max }) => pct <= max) || SIGNAL_ICONS[SIGNAL_ICONS.length - 1];
	return L.resource(`view/modem/icons/${icn}`);
}

/**
 * Calculate DL/UL frequencies and band name for LTE mode.
 * @param {number} rfcn
 * @returns {{ dlfreq: number, ulfreq: number, band: string, frdl: number, frul: number, offset: number }}
 */
function calcLteBand(rfcn) {
	const b = LTE_BANDS.find(b => rfcn >= b.min && rfcn <= b.max);
	if (!b) return { frdl: 0, frul: 0, offset: 0, band: String(rfcn), dlfreq: 0, ulfreq: 0 };
	const dlfreq = b.frdl + (rfcn - b.offset) / 10;
	const ulfreq = b.frul + (rfcn - b.offset) / 10;
	return { ...b, dlfreq, ulfreq };
}

/**
 * Calculate DL/UL frequencies and band name for non-LTE modes.
 * @param {number} rfcn
 * @returns {{ dlfreq: number, ulfreq: number, band: string }}
 */
function calcNonLteBand(rfcn) {
	const match = NON_LTE_BANDS.find(b => b.condition(rfcn));
	return match ? match.calc(rfcn) : { ulfreq: 0, dlfreq: 0, band: String(rfcn) };
}

/**
 * Build a LAC/CID/eNB/Cell/PCI label and value string.
 * @param {Object} modem - Single modem entry
 * @returns {{ namecid: string, lactac: string }}
 */
function buildCellId(modem) {
	const { enbid, cell, pci, lac, cid } = modem;
	const parts   = [lac, cid];
	let namecid   = 'LAC/CID';

	if (enbid) {
		parts.push(enbid);
		namecid += '/eNB ID';

		if (cell) {
			parts.push(cell);
			namecid += '/Cell';

			if (pci) {
				parts.push(pci);
				namecid += '/PCI';
			}
		}
	}

	const lactac = parts.join(' / ');
	return { namecid, lactac };
}

/**
 * Derive mode-specific labels and carrier aggregation info.
 * @param {Object} modem   - Single modem entry
 * @param {string} netmode
 * @param {string} band
 * @param {number} bw
 * @returns {{ carrier, bcc, bca, bwDisplay, namech, namesnr, namecid, lactac, namebnd }}
 */
function buildModeInfo(modem, netmode, band, bw) {
	let carrier  = '';
	let bcc, bca, bwDisplay, namech, namesnr, namecid, lactac;

	if (netmode === 'LTE' || netmode === 'LTE+NR') {
		const calte = modem.lteca;
		carrier = (netmode === 'LTE' && calte > 0) ? '+' : '';
		namech  = 'EARFCN';
		namesnr = 'SINR';
	
		if (calte > 0) {
			bwDisplay = modem.bwca;
			bcc       = ` B${band}${modem.scc}`;
			bca       = bwDisplay ? ` / ${bwDisplay} MHz` : '';
		} else {
			bwDisplay = bw;
			bcc       = ` B${band}`;
			bca       = bw ? ` / ${bw} MHz` : '';
		}

		const cellInfo = buildCellId(modem);
		namecid = cellInfo.namecid;
		lactac  = cellInfo.lactac;

	} else if (UMTS_MODES.test(netmode)) {
		namech  = 'UARFCN';
		namesnr = 'ECIO';
		namecid = 'LAC/CID';
		lactac  = `${modem.lac} / ${modem.cid}`;
		bcc     = ` ${band}`;

	} else {
		namech  = 'ARFCN';
		namesnr = 'SINR/ECIO';
		namecid = 'LAC/CID';
		lactac  = `${modem.lac} / ${modem.cid}`;
		bcc     = ` ${band}`;
	}

	const namebnd = bw
		? _('Network/Band/Bandwidth')
		: _('Network/Band');

	return { carrier, bcc, bca: bca || '', bwDisplay, namech, namesnr, namecid, lactac, namebnd };
}

/**
 * Set textContent of element by id (if it exists).
 * @param {string} id
 * @param {string} text
 */
function setText(id, text) {
	const el = getEl(id);
	if (el) el.textContent = text;
}

/**
 * Set innerHTML of element by id (if it exists).
 * @param {string} id
 * @param {string} html
 */
function setHtml(id, html) {
	const el = getEl(id);
	if (el) el.innerHTML = html;
}

/**
 * Update a signal progress bar, hiding its parent row if value is missing.
 * @param {string} elId    - Element id (without modem index)
 * @param {number} idx     - Modem index
 * @param {*}      rawVal  - Raw value from modem JSON
 * @param {string} unit    - Unit suffix, e.g. ' dBm'
 * @param {string} type    - Key in PROGRESS_CONFIG
 * @param {number} boundary
 */
function updateSignalBar(elId, idx, rawVal, unit, type, boundary) {
	const el = getEl(elId + idx);
	if (!el) return;
	const missing = !rawVal || rawVal === '--' || rawVal === '';
	setRowVisible(el, !missing);
	if (!missing) {
		updateProgressBar(type, rawVal + unit, boundary, idx);
	}
}

// Main view

return view.extend({

	load: function() {
		return L.resolveDefault(fs.exec_direct('/usr/bin/modeminfo'), '{"modem": []}');
	},

	polldata: poll.add(function() {
		return L.resolveDefault(fs.exec_direct('/usr/bin/modeminfo'), '{"modem": []}')
			.then(function(res) {
				let json;
				try {
					json = JSON.parse(res);
				} catch (e) {
					console.error('modeminfo: JSON parse error', e);
					return;
				}

				if (!json || !Array.isArray(json.modem)) return;

				for (let i = 0; i < json.modem.length; i++) {
					const modem   = json.modem[i];
					const netmode = modem.mode  || '';
					const rfcn    = modem.arfcn || 0;

					// Band / Frequency
					let dlfreq, ulfreq, band, bw;

					if (netmode === 'LTE' || netmode === 'LTE+NR') {
						({ dlfreq, ulfreq, band } = calcLteBand(rfcn));
						const bandwidths = [1.4, 3, 5, 10, 15, 20];
						bw = bandwidths[modem.bwdl] || '';
					} else {
						({ dlfreq, ulfreq, band } = calcNonLteBand(rfcn));
						bw = '';
					}

					const arfcnStr = `${rfcn} (${dlfreq} / ${ulfreq} MHz)`;

					// Registration
					const rg  = Number(modem.reg);
					const reg = REG_STATUSES.get(rg) || _('No Data');

					// Signal icon
					const icon = resolveSignalIcon(modem.csq_per || 0);

					// Mode-specific labels
					const { carrier, bcc, bca, namech, namesnr, namecid, lactac, namebnd } =
						buildModeInfo(modem, netmode, band, bw);

					// DOM updates
					setHtml('status' + i,  formatModemStatus(modem, icon, reg));

					const modeEl = getEl('mode' + i);
					if (modeEl) {
						// FIX: was `signal = 0` (assignment), must be `=== 0`
						if (modem.signal === 0 || modem.signal === '' || !netmode) {
							modeEl.textContent = '--';
						} else {
							modeEl.textContent = `${netmode}${carrier} /${bcc}${bca}`;
						}
					}

					setText('namebnd' + i, namebnd);
					setText('chname'  + i, namech);
					setText('namecid' + i, namecid);
					setText('snrname' + i, namesnr);
					setHtml('arfcn'   + i, arfcnStr);
					setHtml('lac'     + i, lactac);

					// Progress bars
					// RSSI always visible if element exists
					if (getEl('rssi' + i)) {
						if (!modem.rssi || modem.rssi === '') {
							setText('rssi' + i, '--');
						} else {
							updateProgressBar('rssi', modem.rssi + ' dBm', -110, i);
						}
					}

					// SINR / ECIO (shared element, hidden when unavailable)
					updateSignalBar(
						'sinr', i, modem.sinr, ' dB',
						netmode === 'LTE' ? 'sinr' : 'ecio',
						netmode === 'LTE' ? -20 : -24
					);

					// RSRP (hidden when unavailable)
					updateSignalBar('rsrp', i, modem.rsrp, ' dBm', 'rsrp', -140);

					// RSRQ (hidden when unavailable)
					updateSignalBar('rsrq', i, modem.rsrq, ' dB',  'rsrq', -20);
				}
			});
	}),

	render: function(data) {
		let json;
		try {
			json = JSON.parse(data);
		} catch (e) {
			json = { modem: [] };
		}

		const m = new form.Map('modeminfo', _('Modeminfo: Network'), _('Cellular network'));
		const s = m.section(form.TypedSection, 'general', null);
		s.anonymous = true;

		for (let i = 0; i < json.modem.length; i++) {
			const idx      = i + 1;
			const statusId = 'status' + i;
			const modeId   = 'mode'   + i;
			const namebndId= 'namebnd'+ i;
			const chnameId = 'chname' + i;
			const namecidId= 'namecid'+ i;
			const arfcnId  = 'arfcn'  + i;
			const lacId    = 'lac'    + i;
			const rssiId   = 'rssi'   + i;
			const sinrId   = 'sinr'   + i;
			const snrnameId= 'snrname'+ i;
			const rsrpId   = 'rsrp'   + i;
			const rsrqId   = 'rsrq'   + i;

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
								E('td', { 'class': 'td left', 'width': '50%' }, [_('Operator')]),
								E('td', { 'class': 'td left', 'id': statusId }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-1' }, [
								E('td', { 'class': 'td left', 'width': '50%', 'id': namebndId }, [_('Network/Band')]),
								E('td', { 'class': 'td left', 'id': modeId }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-2' }, [
								E('td', { 'class': 'td left', 'width': '50%', 'id': chnameId }, [_('E/U/ARFCN')]),
								E('td', { 'class': 'td left', 'id': arfcnId }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-1' }, [
								E('td', { 'class': 'td left', 'width': '50%', 'id': namecidId }, [_('LAC/CID')]),
								E('td', { 'class': 'td left', 'id': lacId }, ['--']),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-2' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('RSSI')]),
								E('td', { 'class': 'td left' },
									E('div', { 'id': rssiId, 'class': 'cbi-progressbar', 'title': '--' }, E('div'))
								),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-1' }, [
								E('td', { 'class': 'td left', 'width': '50%', 'id': snrnameId }, [_('SINR/EcIO')]),
								E('td', { 'class': 'td left' },
									E('div', { 'id': sinrId, 'class': 'cbi-progressbar', 'title': '--' }, E('div'))
								),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-2' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('RSRP')]),
								E('td', { 'class': 'td left' },
									E('div', { 'id': rsrpId, 'class': 'cbi-progressbar', 'title': '--' }, E('div'))
								),
							]),
							E('tr', { 'class': 'tr cbi-rowstyle-1' }, [
								E('td', { 'class': 'td left', 'width': '50%' }, [_('RSRQ')]),
								E('td', { 'class': 'td left' },
									E('div', { 'id': rsrqId, 'class': 'cbi-progressbar', 'title': '--' }, E('div'))
								),
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

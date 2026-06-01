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
	Modified: 5G NR support with combined band display
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
const PROGRESS_CONFIG = {
	rssi: {
		selector: '#rssi',
		min: -110, max: -50,
		calc: (vn, mn) => Math.floor(100 * (1 - (-50 - vn) / (-50 - mn))),
	},
	rsrp: {
		selector: '#rsrp',
		min: -140, max: -50,
		calc: (vn, mn) => Math.floor(120 * (1 - (-50 - vn) / (-70 - mn))),
	},
	sinr: {
		selector: '#sinr',
		min: -20, max: 30,
		calc: (vn, mn) => Math.floor(100 - (100 * (1 - ((mn - vn) / (mn - 30))))),
	},
	rsrq: {
		selector: '#rsrq',
		min: -20, max: 0,
		calc: (vn, mn) => Math.floor(115 - (100 / mn) * vn),
	},
	ecio: {
		selector: '#sinr',
		min: -24, max: 0,
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

// 5G NR bands for FR1 (3GPP TS 38.104)
const NR_BANDS = [
	{ min: 422000, max: 434000, name: '1',  dlLow: 2110, offset: 422000 },
	{ min: 361000, max: 376000, name: '3',  dlLow: 1805, offset: 361000 },
	{ min: 173800, max: 178800, name: '5',  dlLow: 869,  offset: 173800 },
	{ min: 524000, max: 538000, name: '7',  dlLow: 2620, offset: 524000 },
	{ min: 185000, max: 192000, name: '8',  dlLow: 925,  offset: 185000 },
	{ min: 158200, max: 164200, name: '20', dlLow: 791,  offset: 158200 },
	{ min: 499200, max: 538000, name: '41', dlLow: 2496, offset: 499200, tdd: true },
	{ min: 653333, max: 680000, name: '77', dlLow: 3300, offset: 620000, tdd: true },
	{ min: 620000, max: 653332, name: '78', dlLow: 3300, offset: 620000, tdd: true },
	{ min: 693334, max: 733333, name: '79', dlLow: 4400, offset: 693334, tdd: true },
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

// Helper functions
function getEl(id) {
	return document.getElementById(id);
}

function setRowVisible(el, show) {
	const row = el && el.parentElement && el.parentElement.parentElement;
	if (row) row.style.display = show ? '' : 'none';
}

function updateProgressBar(type, value, max, idx) {
	const config = PROGRESS_CONFIG[type];
	if (!config) return;

	const pg = document.querySelector(`${config.selector}${idx}`);
	if (!pg) return;

	const vn = Math.max(config.min, Math.min(config.max, parseInt(value) || 0));
	const mn = parseInt(max) || 100;
	const pc = Math.min(100, Math.max(0, config.calc(vn, mn)));

	pg.firstElementChild.style.width = `${pc}%`;
	pg.firstElementChild.style.animationDirection = 'reverse';
	pg.setAttribute('title', String(value));
}

function formatDistance(dist) {
	if (!dist || dist === '--' || dist === '' || dist === '0.00') return '';
	return ' ~' + dist + ' km';
}

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

function resolveSignalIcon(pct) {
	const { icn } = SIGNAL_ICONS.find(({ max }) => pct <= max) || SIGNAL_ICONS[SIGNAL_ICONS.length - 1];
	return L.resource(`view/modem/icons/${icn}`);
}

function calcLteBand(rfcn) {
	const b = LTE_BANDS.find(b => rfcn >= b.min && rfcn <= b.max);
	if (!b) return { band: String(rfcn), dlfreq: 0, ulfreq: 0 };
	const dlfreq = b.frdl + (rfcn - b.offset) / 10;
	const ulfreq = b.frul + (rfcn - b.offset) / 10;
	return { band: b.band, dlfreq, ulfreq };
}

function calcNrBand(nrarfcn) {
	const b = NR_BANDS.find(b => nrarfcn >= b.min && nrarfcn <= b.max);
	if (!b) return { band: String(nrarfcn), dlfreq: 0 };
	const dlfreq = b.dlLow + (nrarfcn - b.offset) / 1000;
	return { band: b.name, dlfreq };
}

function calcNonLteBand(rfcn) {
	const match = NON_LTE_BANDS.find(b => b.condition(rfcn));
	return match ? match.calc(rfcn) : { band: String(rfcn), dlfreq: 0, ulfreq: 0 };
}

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

function buildModeInfo(modem, netmode, band, bw) {
	let carrier = '';
	let bcc, bca, bwDisplay, namech, namesnr, namecid, lactac;

	if (netmode === 'LTE' || netmode === 'LTE+NR') {
		const calte = modem.lteca;
		carrier = (netmode === 'LTE' && calte > 0) ? '+' : '';
		namech  = 'EARFCN';
		namesnr = 'SINR';

		if (calte > 0) {
			bwDisplay = modem.bwca;
			let sccFormatted = modem.scc || '';
			if (netmode === '5GNR' && sccFormatted) {
				sccFormatted = sccFormatted.replace(/\+(\d+)/g, '+n$1');
				if (sccFormatted.startsWith('+n')) {
					sccFormatted = sccFormatted.substring(1);
				}
				if (sccFormatted) sccFormatted = '+' + sccFormatted;
			}
			bcc = ` ${band}${sccFormatted}`;
			bca = bwDisplay ? ` / ${bwDisplay} MHz` : '';
		} else {
			bwDisplay = bw;
			bcc = ` ${band}`;
			if (netmode === 'LTE+NR' && bw) {
				bca = ` / ${bw} MHz`;
			} else if (bw) {
				bca = ` / ${bw} MHz`;
			} else {
				bca = '';
			}
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

function setText(id, text) {
	const el = getEl(id);
	if (el) el.textContent = text;
}

function setHtml(id, html) {
	const el = getEl(id);
	if (el) el.innerHTML = html;
}

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
					const nrArfcn = modem.nr_arfcn || 0;

					// Band / Frequency
					let lteBand = '', nrBand = '';
					let lteDlfreq = 0, nrDlfreq = 0;
					let band = '', bw = '';

					// LTE band and frequency (for LTE and LTE+NR)
					if (netmode === 'LTE' || netmode === 'LTE+NR') {
						const lte = calcLteBand(rfcn);
						lteBand = `B${lte.band}`;
						lteDlfreq = lte.dlfreq;
					}

					// NR band (for LTE+NR and 5G NR)
					if (netmode === '5GNR' && nrArfcn) {
						const nr = calcNrBand(nrArfcn);
						nrBand = `n${nr.band}`;
						nrDlfreq = nr.dlfreq;
					}

					// Combined band display
					let bandDisplay = '';
					if (lteBand && nrBand) {
						bandDisplay = `${lteBand} + ${nrBand}`;
					} else if (lteBand) {
						bandDisplay = lteBand;
					} else if (nrBand) {
						bandDisplay = nrBand;
					} else {
						// Fallback for UMTS/GSM
						const nonLte = calcNonLteBand(rfcn);
						bandDisplay = nonLte.band;
						lteDlfreq = nonLte.dlfreq;
					}

					// Bandwidth handling
					const lteBandwidths = [1.4, 3, 5, 10, 15, 20];
					const nrBandwidths = [5, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80, 90, 100];
					if (netmode === 'LTE' || netmode === 'LTE+NR') {
						if (modem.bwca && modem.bwca > 0) {
							bw = modem.bwca;
						} else {
							bw = lteBandwidths[modem.bwdl] || '';
						}
					} else if (netmode === '5GNR') {
						if (modem.bwca && modem.bwca > 0) {
							bw = modem.bwca;
						} else if (modem.bwdl) {
							bw = nrBandwidths[modem.bwdl] || '';
						}
					}

					// Frequency display - only primary (LTE or NR)
					let arfcnDisplay = '';
					if (lteDlfreq) {
						arfcnDisplay = `${rfcn} (${lteDlfreq.toFixed(1)} MHz)`;
					} else if (nrDlfreq && nrArfcn) {
						arfcnDisplay = `${nrArfcn} (${nrDlfreq.toFixed(1)} MHz)`;
					}

					// Registration
					const rg  = Number(modem.reg);
					const reg = REG_STATUSES.get(rg) || _('No Data');

					// Signal icon
					const icon = resolveSignalIcon(modem.csq_per || 0);

					// Build mode info for CA and other labels
					const { carrier, bcc, bca, namech, namesnr, namecid, lactac, namebnd } =
						buildModeInfo(modem, netmode, bandDisplay, bw);

					// DOM updates
					setHtml('status' + i, formatModemStatus(modem, icon, reg));

					const modeEl = getEl('mode' + i);
					if (modeEl) {
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
					setHtml('arfcn'   + i, arfcnDisplay);
					setHtml('lac'     + i, lactac);

					// Progress bars
					if (getEl('rssi' + i)) {
						if (!modem.rssi || modem.rssi === '') {
							setText('rssi' + i, '--');
						} else {
							updateProgressBar('rssi', modem.rssi + ' dBm', -110, i);
						}
					}

					updateSignalBar(
						'sinr', i, modem.sinr, ' dB',
						netmode === 'LTE' ? 'sinr' : 'ecio',
						netmode === 'LTE' ? -20 : -24
					);

					updateSignalBar('rsrp', i, modem.rsrp, ' dBm', 'rsrp', -140);
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

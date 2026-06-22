'use strict';
'require form';
'require fs';
'require ui';
'require uci';
'require view';
'require poll';
'require dom';

return view.extend({

	render: function(modems) {
		var m, s, o;

		m = new form.Map('ssw', _('SSW - SIM Card switch'));

		s = m.section(form.TypedSection, 'modem', _('Modem'));
		s.anonymous = true;
		s.addremove = false;

		o = s.option(form.ListValue, 'value', _('State'),
			_('Enable the modem upon router startup.'));
		o.value(1, _('Enable'));
		o.value(0, _('Disable'));

		s = m.section(form.TypedSection, 'sim', _('SIM Slot'));
		s.anonymous = true;
		s.addremove = false;

		o = s.option(form.ListValue, 'value', _('Default SIM Slot'),
			_('Select the primary slot. The other becomes the backup.'));
		o.value(1, _('SLOT 1'));
		o.value(0, _('SLOT 2'));

		// ── Failover section ──────────────────────────────────────────────
		s = m.section(form.TypedSection, 'failover',
			_('Signal-based failover'),
			_('Monitors signal quality (RSRP/RSCP) and link state via mwan3.<br />' +
			  'Switches to the reserve SIM when the average signal drops below the threshold ' +
			  'or the interface loses connectivity. Operates independently of the scheduled switch.'));
		s.anonymous = true;
		s.addremove = false;

		o = s.option(form.Flag, 'enable', _('Enable failover'),
			_('Enable automatic SIM switching based on signal quality and link state.'));

		o = s.option(form.Value, 'apn1', _('APN — default SIM'),
			_('Access Point Name used when operating on the default SIM slot.<br />' +
			  'Applied automatically when reverting from the reserve SIM.'));
		o.placeholder = 'internet';
		o.depends({enable: '1'});

		o = s.option(form.Value, 'apn2', _('APN — reserve SIM'),
			_('Access Point Name used when switching to the reserve SIM slot.'));
		o.placeholder = 'internet';
		o.depends({enable: '1'});

		o = s.option(form.Flag, 'revert', _('Auto-revert to default SIM'),
			_('Automatically switch back to the default SIM after a period of time.<br />' +
			  'The revert timeout starts at <code>2 × (Interval × Probes)</code> seconds<br />' +
			  'and doubles with each consecutive failed attempt.'));
		o.depends({enable: '1'});

		o = s.option(form.ListValue, 'rsrp', _('Signal threshold (RSRP/RSCP)'),
			_('Switch to the reserve SIM when the averaged signal falls below this value.<br />' +
			  'For LTE and 5G use <strong>RSRP</strong>; for 3G (WCDMA) use <strong>RSCP</strong>.<br />' +
			  'Typical thresholds: &minus;95 dBm (good), &minus;105 dBm (acceptable), &minus;115 dBm (poor).'));
		for (var rsrp = -120; rsrp <= -50; rsrp++) {
			o.value(rsrp, rsrp + ' ' + _('dBm'));
		}
		o.depends({enable: '1'});

		o = s.option(form.Value, 'interval', _('Check interval'),
			_('How often the daemon polls signal quality and link state.<br />' +
			  'Shorter intervals react faster but increase CPU and modem load.<br />' +
			  'The total evaluation window is <code>Interval × Probes</code>.'));
		o.value('5', 5 + ' ' + _('sec'));
		for (var sec = 10; sec <= 60; sec += 10) {
			o.value(sec, sec + ' ' + _('sec'));
		}
		o.value('2m', 2 + ' ' + _('minute'));
		o.value('5m', 5 + ' ' + _('minute'));
		for (var d = 10; d <= 60; d += 10) {
			o.value(d + 'm', d + ' ' + _('minute'));
		}
		o.value('2h', 2 + ' ' + _('hour'));
		o.value('4h', 4 + ' ' + _('hour'));
		o.depends({enable: '1'});

		o = s.option(form.Value, 'times_rsrp', _('Probes'),
			_('Number of signal samples to average before making a switch decision.<br />' +
			  'More probes smooth out momentary fluctuations but slow reaction time.<br />' +
			  'Total evaluation window: <code>Interval × Probes</code>.<br />' +
			  'Example: <code>Interval = 60 sec</code>, <code>Probes = 5</code> → decision after <code>300 sec</code>.'));
		for (var p = 5; p <= 10; p++) {
			o.value(p, p);
		}
		o.depends({enable: '1'});

		// ── Schedule section ──────────────────────────────────────────────
		s = m.section(form.TypedSection, 'schedule',
			_('Scheduled SIM switch'),
			_('Unconditional switch to the reserve SIM at a fixed time.<br />' +
			  'Choose how often the switch repeats: every day, every N days, or on a specific weekday.<br />' +
			  'Optionally reverts to the default SIM after a set number of minutes.'));
		s.anonymous = true;
		s.addremove = false;

		// Enable flag
		o = s.option(form.Flag, 'enable', _('Enable scheduled switch'));
		o.default = '0';

		// Switch time
		o = s.option(form.Value, 'time_on', _('Switch time (HH:MM)'),
			_('Time of day to switch to the reserve SIM (24 h format, e.g. <code>02:00</code>).'));
		o.placeholder = '02:00';
		o.rmempty = false;
		o.validate = function(section_id, value) {
			if (!/^\d{1,2}:\d{2}$/.test(value))
				return _('Use HH:MM format');
			var parts = value.split(':');
			if (parseInt(parts[0]) > 23 || parseInt(parts[1]) > 59)
				return _('Invalid time value');
			return true;
		};
		o.depends({enable: '1'});

		// Period selector
		o = s.option(form.ListValue, 'period', _('Repeat period'));
		o.value('daily',    _('Every day'));
		o.value('interval', _('Every N days'));
		o.value('weekly',   _('Weekly (specific weekday)'));
		o.default = 'daily';
		o.depends({enable: '1'});

		// Every-N-days field (shown only when period=interval)
		o = s.option(form.Value, 'period_days', _('Repeat every (days)'),
			_('Switch to the reserve SIM every this many days. Minimum 2.'));
		o.placeholder = '3';
		o.datatype = 'uinteger';
		o.default = '3';
		o.validate = function(section_id, value) {
			var n = parseInt(value);
			if (isNaN(n) || n < 2)
				return _('Must be 2 or more');
			return true;
		};
		o.depends({enable: '1', period: 'interval'});

		// Weekday selector (shown only when period=weekly)
		o = s.option(form.ListValue, 'weekday', _('Weekday'),
			_('Day of the week on which the switch fires.'));
		o.value('0', _('Sunday'));
		o.value('1', _('Monday'));
		o.value('2', _('Tuesday'));
		o.value('3', _('Wednesday'));
		o.value('4', _('Thursday'));
		o.value('5', _('Friday'));
		o.value('6', _('Saturday'));
		o.default = '1';
		o.depends({enable: '1', period: 'weekly'});

		// Revert duration
		o = s.option(form.Value, 'duration', _('Revert after (minutes)'),
			_('Minutes until automatic revert to the default SIM.<br />' +
			  'Set to <code>0</code> to stay on the reserve SIM indefinitely.'));
		o.placeholder = '60';
		o.datatype = 'uinteger';
		o.default = '0';
		o.depends({enable: '1'});

		// APN override
		o = s.option(form.Value, 'apn', _('APN for scheduled switch'),
			_('APN to use during the scheduled window.<br />' +
			  'Leave empty to use <em>APN Reserved SIM</em> from Failover settings.'));
		o.placeholder = 'internet';
		o.depends({enable: '1'});

		return m.render();
	}
});

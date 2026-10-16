'use strict';
'require dom';
'require form';
'require ui';
'require uci';
'require view';
'require tools.widgets as widgets';

const RATE_UNITS = [ 'kbit', 'Mbit', 'Gbit' ];

/* split a tc rate like "500Mbit" into [ '500', 'Mbit' ]; unit is null if unparsable */
function splitRate(value) {
	const str = String(value ?? '').trim();
	const m = str.match(/^(\d+(?:\.\d+)?)\s*([A-Za-z]*)$/);

	if (!m)
		return [ str, null ];

	/* a bare number is bit/s for tc, other units are kept as they are */
	return [ m[1], RATE_UNITS.find(u => u.toLowerCase() === m[2].toLowerCase()) ?? (m[2] || 'bit') ];
}

/* number input + unit selector, the value is stored as e.g. "500Mbit" */
const RateInput = ui.AbstractElement.extend({
	__init__(value, options) {
		this.value = value;
		this.options = Object.assign({ optional: true }, options);
	},

	render() {
		const [ num, unit ] = splitRate(this.value);
		const [ pnum, punit ] = splitRate(this.options.placeholder);

		const inputEl = E('input', {
			'type': 'text',
			'class': 'cbi-input-text',
			'inputmode': 'decimal',
			'style': 'min-width:5em',
			'placeholder': pnum,
			'disabled': this.options.disabled ? '' : null,
			'value': num
		});

		const selectEl = E('select', {
			'class': 'cbi-input-select',
			'style': 'flex:0 0 auto; width:5.5em; min-width:0; max-width:none',
			'disabled': this.options.disabled ? '' : null
		});

		this.fillUnits(selectEl, unit ?? punit ?? RATE_UNITS[1]);

		return this.bind(E('div', { 'id': this.options.id }, [
			E('div', { 'class': 'control-group' }, [ inputEl, selectEl ])
		]));
	},

	/* units outside of RATE_UNITS (bps, kibit, ...) are kept so they survive a save */
	fillUnits(selectEl, current) {
		const units = RATE_UNITS.slice();

		if (!units.includes(current))
			units.push(current);

		dom.content(selectEl, units.map(u => E('option', { 'value': u }, u)));
		selectEl.value = current;
	},

	bind(frameEl) {
		const inputEl = frameEl.querySelector('input');
		const selectEl = frameEl.querySelector('select');

		this.node = frameEl;

		this.setUpdateEvents(inputEl, 'keyup', 'blur');
		this.setChangeEvents(inputEl, 'change');
		this.setChangeEvents(selectEl, 'change');
		this.registerEvents(selectEl, 'widget-update', [ 'change' ]);

		dom.bindClassInstance(frameEl, this);

		return frameEl;
	},

	getValue() {
		const num = this.node.querySelector('input').value.trim();

		return (num === '') ? '' : num + this.node.querySelector('select').value;
	},

	setValue(value) {
		const [ num, unit ] = splitRate(value);

		this.node.querySelector('input').value = num;

		if (unit != null)
			this.fillUnits(this.node.querySelector('select'), unit);
	}
});

const RateValue = form.Value.extend({
	renderWidget(section_id, option_index, cfgvalue) {
		return new RateInput((cfgvalue != null) ? cfgvalue : this.default, {
			id: this.cbid(section_id),
			optional: this.optional || this.rmempty,
			datatype: 'ufloat',
			placeholder: this.placeholder,
			disabled: (this.readonly != null) ? this.readonly : this.map.readonly
		}).render();
	}
});

return view.extend({
	load: function() {
		return uci.load('ifb-shaper');
	},

	render: function() {
		let m, s, o;

		m = new form.Map('ifb-shaper', _('IFB Shaper'),
			_('Per-client traffic shaper. Download is shaped on the egress of the interface device, upload on an IFB device (ifb-&lt;device&gt;) that is created automatically.'));

		/* shapers */
		s = m.section(form.GridSection, 'shaper', _('Shapers'),
			_('A shaper is bound to a logical interface; clients refer to it by the name of this section.'));
		s.addremove = true;
		s.anonymous = false;
		s.nodescriptions = true;
		s.modaltitle = _('Shaper');

		s.renderSectionAdd = function(extra_class) {
			const el = form.GridSection.prototype.renderSectionAdd.apply(this, arguments);
			const nameEl = el.querySelector('.cbi-section-create-name');

			ui.addValidator(nameEl, 'uciname', true, function(v) {
				if (uci.get('ifb-shaper', v) != null)
					return _('This name is already used');

				return true;
			}, 'blur', 'keyup');

			return el;
		};

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.default = o.enabled;
		o.editable = true;

		o = s.option(widgets.NetworkSelect, 'interface', _('Interface'));
		o.nocreate = true;
		o.rmempty = false;

		o = s.option(form.DynamicList, 'bypass', _('Bypass networks'),
			_('Traffic to and from these networks skips the client limits and goes to the local class.'));
		o.datatype = 'or(cidr4,cidr6)';
		o.placeholder = '192.168.0.0/24';
		o.modalonly = true;

		o = s.option(RateValue, 'total', _('Total rate'));
		o.placeholder = '1000Mbit';
		o.modalonly = true;

		o = s.option(RateValue, 'down_total', _('Download total'),
			_('Aggregate rate of all clients.'));
		o.placeholder = '400Mbit';
		o.modalonly = true;

		o = s.option(RateValue, 'up_total', _('Upload total'),
			_('Aggregate rate of all clients.'));
		o.placeholder = '300Mbit';
		o.modalonly = true;

		o = s.option(RateValue, 'local_rate', _('Local class rate'));
		o.placeholder = '50Mbit';
		o.modalonly = true;

		o = s.option(RateValue, 'local_ceil', _('Local class ceil'));
		o.placeholder = '500Mbit';
		o.modalonly = true;

		o = s.option(RateValue, 'default_rate', _('Default class rate'),
			_('Traffic that matches no client.'));
		o.placeholder = '1Mbit';
		o.modalonly = true;

		o = s.option(RateValue, 'default_ceil', _('Default class ceil'));
		o.placeholder = '15Mbit';
		o.modalonly = true;

		o = s.option(form.Value, 'leaf_qdisc', _('Leaf qdisc'),
			_('tc arguments of the qdisc attached to every client sub-class, or <code>none</code>.'));
		o.placeholder = 'fq_codel limit 1024 flows 128';
		o.modalonly = true;

		/* clients */
		s = m.section(form.GridSection, 'client', _('Clients'),
			_('Clients are matched from top to bottom: put specific hosts before wide subnets that contain them.'));
		s.addremove = true;
		s.anonymous = false;
		s.nodescriptions = true;
		s.sortable = true;
		s.modaltitle = _('Client');

		s.renderSectionAdd = function(extra_class) {
			const el = form.GridSection.prototype.renderSectionAdd.apply(this, arguments);
			const nameEl = el.querySelector('.cbi-section-create-name');

			ui.addValidator(nameEl, 'uciname', true, function(v) {
				if (uci.get('ifb-shaper', v) != null)
					return _('This name is already used');

				return true;
			}, 'blur', 'keyup');

			return el;
		};

		o = s.option(form.ListValue, 'shaper', _('Shaper'));
		o.rmempty = false;
		o.load = function(section_id) {
			this.keylist = [];
			this.vallist = [];

			uci.sections('ifb-shaper', 'shaper', sh => this.value(sh['.name']));

			return form.ListValue.prototype.load.apply(this, [ section_id ]);
		};

		o = s.option(form.DynamicList, 'address', _('Addresses'),
			_('Hosts or networks (IPv4 and/or IPv6) sharing one limit.'));
		o.datatype = 'ipmask';
		o.placeholder = '192.168.1.0/24';
		o.rmempty = false;

		o = s.option(RateValue, 'down_ceil', _('Download max'));
		o.placeholder = '50Mbit';
		o.rmempty = false;

		o = s.option(RateValue, 'down_rate', _('Download guaranteed'));
		o.placeholder = '1Mbit';
		o.modalonly = true;

		o = s.option(RateValue, 'up_ceil', _('Upload max'));
		o.placeholder = '20Mbit';
		o.rmempty = false;

		o = s.option(RateValue, 'up_rate', _('Upload guaranteed'));
		o.placeholder = '1Mbit';
		o.modalonly = true;

		return m.render();
	}
});

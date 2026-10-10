'use strict';

// Весь I/O в одном месте: uci, ubus, nft. Логика живёт в plan.uc / chain.uc.

import { cursor } from 'uci';
import { connect } from 'ubus';
import * as fs from 'fs';
import { DEFAULTS } from 'mwan3_autobalancer.model';

const BATCH = '/var/run/mwan3-autobalancer.batch';

function as_list(v) {
	return (type(v) == 'array') ? v : (v == null ? [] : [v]);
}

export function load_config() {
	let c = cursor(), s = c.get_all('mwan3_autobalancer', 'main') ?? {};
	let interval = int(s.interval ?? 30), total = int(s.total ?? DEFAULTS.total);
	return {
		...DEFAULTS,
		enabled:  s.enabled == '1',
		observe:  s.observe != '0',
		policy:   s.policy ?? 'balanced',
		interval: interval < 5 ? 5 : interval,
		total:    total < 2 ? DEFAULTS.total : total,
		hyst:     (s.hyst != null) ? (s.hyst * 1.0) : DEFAULTS.hyst
	};
};

// порядок секций interface задаёт id интерфейсов (как в mwan3_update_iface_to_table)
function read_mwan3(policy) {
	let c = cursor(), order = [], members = {};
	c.load('mwan3');
	c.foreach('mwan3', 'interface', s => { push(order, s['.name']); });

	let pol = c.get_all('mwan3', policy);
	for (let m in as_list(pol?.use_member)) {
		let s = c.get_all('mwan3', m);
		if (s?.interface)
			members[s.interface] = int(s.weight ?? 1);
	}
	return { order, members };
}

function read_status() {
	let conn = connect();
	let r = conn ? conn.call('mwan3', 'status', { section: 'interfaces' }) : null;
	return r?.interfaces;
}

function read_chain(chain) {
	let p = fs.popen(`nft -j list chain inet mwan3 ${chain} 2>/dev/null`, 'r');
	if (!p)
		return null;
	let out = p.read('all');
	p.close();
	return length(out) ? json(out) : null;
}

// -> { order, members, status, doc, chain } или { error }
export function collect(cfg) {
	if (!match(cfg.policy, /^[A-Za-z0-9_]+$/))
		return { error: `wrong policy name: ${cfg.policy}` };
	let m = read_mwan3(cfg.policy);
	if (!length(m.members))
		return { error: `not members in ${cfg.policy}` };
	let status = read_status();
	if (status == null)
		return { error: 'mwan3 nor respone ubus (status)' };
	let chain = 'mwan3_policy_' + cfg.policy;
	let doc = read_chain(chain);
	if (doc == null)
		return { error: `${chain} not exist` };
	return { order: m.order, members: m.members, status, doc, chain };
};

export function nft_apply(batch) {
	fs.writefile(BATCH, batch);
	let rc = system(['nft', '-j', '-f', BATCH], 5000);
	fs.unlink(BATCH);
	return rc == 0;
};

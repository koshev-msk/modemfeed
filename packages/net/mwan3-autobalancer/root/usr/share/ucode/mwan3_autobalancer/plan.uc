'use strict';

// Один проход без I/O: статус + живая цепочка + uci -> решение.
// inp = { order, members: { iface: base_weight }, status: { iface: {...} }, doc, chain }

import { DEFAULTS, new_state, quality, targets, integerize, needs_apply } from 'mwan3_autobalancer.model';
import { vmap_rules, widths, name_by_id, rebuild_batch } from 'mwan3_autobalancer.chain';

// Усредняем track_ip, у которых есть живой замер. Нет данных -> null.
export function sample_of(st) {
	if (st == null || st.status != 'online')
		return null;
	let n = 0, lat = 0.0, loss = 0.0;
	for (let t in (st.track_ip ?? [])) {
		if (t.status != 'up' || !(t.latency > 0))
			continue;
		n++;
		lat += t.latency;
		loss += (t.packetloss ?? 0);
	}
	return n ? { latency: lat / n, loss: loss / n } : null;
};

function rule_members(inp, vr) {
	let names = [], list = [];
	for (let e in vr.entries) {
		let name = name_by_id(inp.order, e.id);
		let base = (name != null) ? inp.members[name] : null;
		if (base == null)
			return { error: `id ${e.id}: iface not exist in this policy` };
		push(names, name);
		push(list, { base });
	}
	return { names, list };
}

function fq(q) { return (q == null) ? '-' : sprintf('%.2f', q); }

export function plan(inp, cfg, states) {
	cfg = cfg ?? DEFAULTS;
	let vrs = vmap_rules(inp.doc), lines = [], parts = [], weights = {}, any = false, q = {};

	if (!length(vrs))
		return { batch: null, lines: ['rule not exitst in numgen/vmap (one member or unknown form)'], summary: 'no vmap' };

	for (let n = 0; n < length(vrs); n++) {
		let vr = vrs[n], rm = rule_members(inp, vr);
		if (rm.error) {
			push(lines, `rule ${n}: ${rm.error}`);
			push(parts, `${n}:пропуск`);
			continue;
		}
		for (let i = 0; i < length(rm.names); i++) {
			let name = rm.names[i];
			if (!exists(q, name)) {
				if (!exists(states, name))
					states[name] = new_state();
				q[name] = quality(sample_of((inp.status ?? {})[name]), states[name], cfg);
			}
			rm.list[i].q = q[name];
		}

		let t = targets(rm.list, cfg.total), w = integerize(t, cfg.total), cur = widths(vr);
		for (let i = 0; i < length(rm.names); i++)
			push(lines, sprintf('правило %d: %s вес %d (цель %.1f из %d) q=%s', n, rm.names[i], cur[i], t[i], cfg.total, fq(rm.list[i].q)));

		let apply = (vr.mod != cfg.total) || needs_apply(cur, t, cfg.total, cfg.hyst);
		push(parts, `${n}:` + join(',', map(rm.names, (nm, i) => `${nm}=${w[i]}`)) + (apply ? '*' : ''));
		if (apply) {
			weights[n] = w;
			any = true;
		}
	}

	return {
		batch: any ? rebuild_batch(inp.doc, inp.chain, weights) : null,
		lines,
		summary: join(' ', parts)
	};
};

// Откат к «стоку»: ширины диапазонов = базовые веса из uci
export function restore_plan(inp) {
	let vrs = vmap_rules(inp.doc), weights = {}, any = false;
	for (let n = 0; n < length(vrs); n++) {
		let rm = rule_members(inp, vrs[n]);
		if (rm.error)
			continue;
		weights[n] = map(rm.list, x => x.base);
		any = true;
	}
	return any ? rebuild_batch(inp.doc, inp.chain, weights) : null;
};

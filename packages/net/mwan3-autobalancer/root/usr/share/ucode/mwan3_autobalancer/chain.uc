'use strict';

// Чистая работа с JSON цепочки mwan3_policy_<name> (вывод `nft -j list chain`).
// Никакого I/O: на вход разобранный JSON, на выход структуры или готовый batch.
// Если форма правила не узнана, vmap_rules() вернёт [] и демон ничего не тронет.

// Обратная к mwan3_id2mask: метка + маска -> номер интерфейса
export function mark_to_id(mark, mask) {
	let id = 0, bit_val = 0;
	for (let bit = 0; bit < 32; bit++) {
		if (((mask >> bit) & 1) == 1) {
			if (((mark >> bit) & 1) == 1)
				id |= (1 << bit_val);
			bit_val++;
		}
	}
	return id;
};

// id интерфейса = номер секции interface в /etc/config/mwan3 (с 1), как в mwan3.sh
export function name_by_id(order, id) {
	return order[id - 1];
};

function clone(x) {
	return json(sprintf('%J', x));
}

// Все правила `numgen inc ... vmap` цепочки.
// -> [{ rule, vi, mod, mask, entries: [{ lo, hi, target, mark, id }] }]
export function vmap_rules(doc) {
	let out = [];
	for (let item in (doc?.nftables ?? [])) {
		let rule = item.rule;
		if (rule == null || type(rule.expr) != 'array')
			continue;

		let vi = -1, mask = null;
		for (let i = 0; i < length(rule.expr); i++) {
			let e = rule.expr[i];
			if ((e.vmap?.key?.numgen?.mode == 'inc' || e.vmap?.key?.numgen?.mode == 'random') && type(e.vmap?.data?.set) == 'array')
				vi = i;
			let l = e.match?.left;
			if (type(l) == 'object' && type(l['&']) == 'array')
				mask = l['&'][1];
		}
		if (vi < 0 || mask == null)
			continue;

		let entries = [], ok = true;
		for (let pair in rule.expr[vi].vmap.data.set) {
			let k = pair[0], v = pair[1];
			let rg = (type(k) == 'object') ? k.range : [k, k];
			let m = match(v?.jump?.target ?? '', /^mwan3_or_meta_0x([0-9a-fA-F]+)$/);
			if (type(rg) != 'array' || m == null) { ok = false; break; }
			let mark = hex(m[1]);
			push(entries, { lo: rg[0], hi: rg[1], target: v.jump.target, mark, id: mark_to_id(mark, mask) });
		}
		if (!ok || length(entries) < 2)
			continue;

		push(out, { rule, vi, mod: rule.expr[vi].vmap.key.numgen.mod, mask, entries });
	}
	return out;
};

// Текущие ширины диапазонов (вес каждого члена в живой цепочке)
export function widths(vr) {
	return map(vr.entries, e => e.hi - e.lo + 1);
};

// Новый batch: flush + add всех правил цепочки по порядку, у vmap-правил подменены
// диапазоны и mod. weights: { <индекс vmap-правила>: [w1, w2, ...] } в порядке entries.
// Возвращает строку JSON для `nft -j -f -` или null при несоответствии.
export function rebuild_batch(doc, chain, weights) {
	let vrs = vmap_rules(doc), batch = [], fam, tbl;

	for (let item in doc.nftables) {
		if (item.chain?.name == chain) { fam = item.chain.family; tbl = item.chain.table; }
	}
	if (fam == null)
		return null;

	push(batch, { flush: { chain: { family: fam, table: tbl, name: chain } } });

	let ri = 0;
	for (let item in doc.nftables) {
		if (item.rule == null)
			continue;
		let r = clone(item.rule);
		delete r.handle;

		for (let n = 0; n < length(vrs); n++) {
			if (sprintf('%J', vrs[n].rule) != sprintf('%J', item.rule))
				continue;
			let w = weights[n];
			if (w == null)
				break;
			if (length(w) != length(vrs[n].entries))
				return null;
			let set = [], run = 0;
			for (let i = 0; i < length(w); i++) {
				if (w[i] < 1)
					return null;
				push(set, [{ range: [run, run + w[i] - 1] }, { jump: { target: vrs[n].entries[i].target } }]);
				run += w[i];
			}
			r.expr[vrs[n].vi].vmap.data.set = set;
			r.expr[vrs[n].vi].vmap.key.numgen.mod = run;
			break;
		}
		push(batch, { add: { rule: r } });
	}
	return sprintf('%J', { nftables: batch });
};

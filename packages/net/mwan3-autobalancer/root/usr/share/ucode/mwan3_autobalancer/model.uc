'use strict';

// Чистая логика без I/O: качество линка -> веса. Тестируется на хосте.
// Внимание: в ucode int / int делится нацело, поэтому везде умножаем на 1.0.

export const DEFAULTS = {
	total:    20,    // сумма целых весов в цепочке (шаг 5%)
	alpha:    0.3,   // EWMA для задержки
	drift:    0.01,  // как быстро «обычная» задержка ползёт вверх
	lat_free: 1.5,   // до x1.5 от обычной задержки штрафа нет
	loss_alpha: 0.1, // EWMA потерь: одиночный потерянный пинг почти не виден
	loss_free: 5,    // сглаженные потери до 5% штрафа не дают
	loss_div: 50,    // каждые 50% сверх порога снимают весь вес (с запасом 0.9)
	hyst:     1.0    // применять, когда цель ушла от живого веса на >= 1 шаг
};

export function new_state() {
	return { lat: null, base: null, loss: null };
};

// sample: { latency (мс), loss (%) } из mwan3track; st: состояние линка (меняется)
// Возвращает q в (0..1] или null, если данных нет.
export function quality(sample, st, cfg) {
	cfg = cfg ?? DEFAULTS;
	if (sample == null || sample.latency == null || sample.latency <= 0)
		return null;

	let lat = sample.latency;
	st.lat = (st.lat == null) ? lat : st.lat + (lat - st.lat) * cfg.alpha;
	st.base = (st.base == null || st.lat < st.base)
		? st.lat
		: st.base + (st.lat - st.base) * cfg.drift;

	let r = (st.lat * 1.0) / st.base;
	let q_lat = (r <= cfg.lat_free) ? 1 : cfg.lat_free / r;

	let loss = (sample.loss ?? 0) * 1.0;
	st.loss = (st.loss == null) ? loss : st.loss + (loss - st.loss) * cfg.loss_alpha;
	let l = (st.loss - cfg.loss_free) / cfg.loss_div;
	if (l < 0) l = 0;
	let q_loss = 1 - (l > 0.9 ? 0.9 : l);

	return q_lat * q_loss;
};

// members: [{ base (вес из uci), q (или null) }] -> вещественные цели в масштабе total
export function targets(members, total) {
	let sum = 0, t = [];
	for (let m in members)
		sum += m.base * (m.q == null ? 1 : m.q);
	for (let m in members)
		push(t, sum > 0 ? (total * 1.0) * m.base * (m.q == null ? 1 : m.q) / sum : (total * 1.0) / length(members));
	return t;
};

// Метод наибольших остатков, минимум 1 на участника
export function integerize(t, total) {
	let n = length(t), w = [], r = [], s = 0;
	for (let i = 0; i < n; i++) {
		let f = int(t[i]);
		push(w, f < 1 ? 1 : f);
		push(r, t[i] - f);
		s += w[i];
	}
	for (let g = 0; s < total && g < total; g++) {
		let b = 0;
		for (let i = 1; i < n; i++)
			if (r[i] > r[b]) b = i;
		w[b]++; r[b] = -1; s++;
	}
	for (let g = 0; s > total && g < total; g++) {
		let b = 0;
		for (let i = 1; i < n; i++)
			if (w[i] > w[b]) b = i;
		if (w[b] <= 1) break;
		w[b]--; s--;
	}
	return w;
};

// cur: ширины диапазонов в живой цепочке (любой масштаб)
export function needs_apply(cur, t, total, hyst) {
	hyst = hyst ?? DEFAULTS.hyst;
	let s = 0;
	for (let x in cur) s += x;
	if (s <= 0 || length(cur) != length(t))
		return true;
	for (let i = 0; i < length(t); i++) {
		let d = (cur[i] * 1.0) * total / s - t[i];
		if ((d < 0 ? -d : d) >= hyst)
			return true;
	}
	return false;
};

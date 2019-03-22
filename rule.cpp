// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <algorithm>
#include "rule.h"
#ifdef DEBUG
#include "driver.h"
#endif
using namespace std;

#define from_int_and(x, y, o, r) r = bdd_and(r, from_int(x, y, o))
#define vecfill(v,x,y,z) fill((v).begin() + (x), (v).begin() + (y), z)
#define symcat(x, y) ((x).push_back(y), (x))
#define err_proof	"proof extraction yet unsupported for programs "\
			"with negation or deletion."

int_t null, openp, closep;
template<typename T, typename V>
V& cat(V& v, const T& t) { return v.push_back(t), v; }

template<typename V>
V& cat(V& v, const V& t, size_t off = 0, size_t loff = 0, size_t roff = 0) {
	return v.insert(v.end()-off, t.begin()+loff, t.end()-roff), v;
}

size_t fact(term v, size_t bits) {
	size_t r = T;
	map<int_t, size_t> m;
	auto it = m.end();
	for (size_t j = 0; j != v.size() - 1; ++j)
		if (v[j+1] >= 0) from_int_and(v[j+1], bits, j * bits, r);
		else if (m.end() == (it = m.find(v[j+1]))) m.emplace(v[j+1],j);
		else for (size_t b = 0; b!=bits; ++b)
			r = bdd_and(r, from_eq(j*bits+b, it->second*bits+b));
	return v[0] < 0 ? bdd_and_not(T, r) : r;
}

size_t varcount(const matrix& v) { // bodies only
	set<int_t> vars;
	for (const term& x : v) 
		for (size_t i = 1; i != x.size(); ++i) 
			if (x[i] < 0) vars.emplace(x[i]);
	return vars.size();
}

rule::body::body(term &t, size_t ar, size_t bits, size_t dsz, size_t nvars)
	: neg(t[0] < 0) { // init, sel, ex and local eq
	map<int_t, size_t> m;
	size_t b, j;
	t.erase(t.begin()), 
	ex.resize(bits*ar,false), perm.resize((ar+nvars)*bits);
	for (b = 0; b != (ar + nvars) * bits; ++b) perm[b] = b;
	for (j = 0; j != ar; ++j)
		if (t[j] >= 0)
			from_int_and(t[j], bits, j * bits, sel);
	for (j = 0; j != ar; ++j) from_arg(t[j], j, bits, dsz, m);
}

void rule::body::from_arg(int_t vij, size_t j, size_t bits, size_t dsz,
	map<int_t, size_t>& m) {
	auto it = m.end();
	vector<array<size_t, 2>> eq;
	set<int_t> exclude = { pad, openp, closep };
	if (vij >= 0) vecfill(ex, j * bits, (j+1) * bits, true);
	else if ((it = m.find(vij)) != m.end()) { //seen var
		vecfill(ex, j * bits, (j+1) * bits, true);
		for (size_t b = 0; b != bits; ++b)
			eq.emplace_back(array<size_t, 2>
				{j*bits+b, it->second*bits+b});
	} else	m.emplace(vij, j), from_range(dsz, bits, j*bits, exclude, sel);
	for (size_t i = 0; i != eq.size(); ++i) {
		if (!(i % 8)) eqs.push_back(T);
		eqs.back() = bdd_and(eqs.back(), from_eq(eq[i][0], eq[i][1]));
	}
}

size_t rule::body::varbdd(size_t db, lp::step& p) const {
	auto it = (neg ? p.neg : p.pos).find({sel, ex});
	//DBG(printbdd(wcout <<"sel: " << endl, sel)<<endl;)
	if (it != (neg?p.neg:p.pos).end()) return bdd_permute(it->second, perm);
	size_t r = (neg?bdd_and_not:bdd_and)(sel, db), n;
	if (r == F) goto ret;
	n = eqs.size();
	while (n) if (F == (r = bdd_and(r, eqs[--n]))) goto ret;
	r = bdd_ex(r, ex);
ret:	(neg ? p.neg : p.pos).emplace(make_pair(r, ex), r);
	return r == F ? F : bdd_permute(r, perm);
}

rule::rule(matrix v, size_t bits, size_t dsz, bool proof) {
	size_t i, j, b, ar = v[0].size() - 1, k = ar, nvars;
	assert(v.size() > 1);
	neg = v[0][0] < 0, v[0].erase(v[0].begin()), nvars = varcount(v);
	for (i=1; i!=v.size(); ++i) bd.emplace_back(v[i], ar, bits, dsz, nvars);
	vector<array<size_t, 2>> heq;
	map<int_t, size_t> m;
	auto it = m.end();
	for (j = 0; j != ar; ++j) // hsym
		if (v[0][j] >= 0) from_int_and(v[0][j], bits, j * bits, hsym);
		else if (m.end() == (it=m.find(v[0][j]))) m.emplace(v[0][j], j);
		else for (b = 0; b!=bits; ++b)
			heq.emplace_back(array<size_t, 2>
				{j * bits + b, it->second * bits + b});
	for (j = 0; j != heq.size(); ++j) {
		if (!(j % 8)) eqs.push_back(T);
		eqs.back() = bdd_and(eqs.back(), from_eq(heq[j][0], heq[j][1]));
	}
	for (i = 0; i != v.size() - 1; ++i) // var permutations
		for (j = 0; j != ar; ++j)
			if (v[i+1][j] < 0) {
				if ((it = m.find(v[i+1][j])) == m.end())
					it = m.emplace(v[i+1][j], k++).first;
				for (b = 0; b != bits; ++b)
					bd[i].perm[b+j*bits]=b+it->second*bits;
			}
	vars_arity = k;

	if (!proof) return;
	if (neg) er(err_proof);
	for (const body& b : bd) if (b.neg) er(err_proof);

	term vars, prule, bprule, x, y;
	set<size_t> vs;
	for (int_t t : v[0]) if (t < 0) vs.insert(t);
	cat(cat(vars, 1), v[0]), cat(cat(prule, 1), openp), cat(bprule, 1);
	//for (auto x : m) if (x.second >= ar) cat(vars, x.first);
	for (i = 1; i != v.size(); ++i)
		for (int_t t : v[i])
			if (t < 0 && vs.find(t) == vs.end())
				vs.insert(t), cat(vars, t);
	//for (term& t : v) while (t[t.size()-1] == pad) t.erase(t.end()-1);
	for (i = 0; i != v.size(); ++i) cat(prule, v[i]);
	cat(prule, closep), cat(bprule, v[0]), cat(bprule, openp);
	for (i = 1; i != v.size(); ++i) cat(bprule, v[i]);
	cat(bprule, closep);

	proof1 = {{prule},{vars}};
	matrix r = { bprule, prule, cat(cat(cat(y={1}, openp), v[0]), closep) };
	for (i = 1; i != v.size(); ++i)
		proof2.insert({
			cat(cat(cat(x={1}, openp), v[i]), closep),
			prule, r[2]}),
//			cat(cat(cat(y={1}, openp), v[0]), closep)}),
		r.push_back(cat(cat(cat(x={1}, openp), v[i]), closep));
	proof2.insert(move(r));
/*	wcout << v << endl << vars << endl << endl;
	drv->printbdd(wcout, v)<<endl, drv->printbdd(wcout, proof1)<<endl,
	drv->printbdd(wcout, proof2), exit(0);*/
}

size_t rule::fwd(size_t db, size_t bits, size_t ar, lp::step& s) {
	size_t vars = T;
/*	vector<size_t> v;
	if (bd.size() == 1) {
		if (F == (vars = bd[0].varbdd(db, s))) return false;
		for (size_t n = eqs.size(); n; ++n)
			if (F == (vars = bdd_and(vars, eqs[n]))) return false;
		vars = bdd_and(vars, hsym);
		goto ret;
	}
	if (bd.size() == 2) {
		if (F == (vars = bdd_and(bd[0].varbdd(db, s),
			bd[1].varbdd(db, s)))) return false;
		for (size_t n = eqs.size(); n; ++n)
			if (F == (vars = bdd_and(vars, eqs[n]))) return false;
		vars = bdd_and(vars, hsym);
		goto ret;
	}*/
	vector<size_t> v(bd.size() + eqs.size() + 1);
	size_t i = 0;
	for (const body& b : bd)
		if (F == (vars = b.varbdd(db, s))) return F;
		else v[i++] = vars;
	for (size_t n : eqs) v[i++] = n;
	v[i] = hsym;
	if (F == (vars = bdd_and_many(v, 0, v.size()))) return F;
	if (!proof2.empty()) p.emplace(vars);
	return bdd_deltail(vars, bits * ar);

/*	for (const body& b : bd)
		if (F == (vars = bdd_and(vars, b.varbdd(db, s)))) return F;
	for (n=eqs.size(); n;) if (F==(vars=bdd_and(vars, eqs[--n]))) return F;
	vars = bdd_and(vars, hsym);
	if (!proof2.empty()) p.emplace(vars);
	return bdd_deltail(vars, bits * ar);*/
}

size_t rule::get_varbdd(size_t bits, size_t ar) const {
	size_t x = T, y = F, n;
	for (size_t z : p) y = bdd_or(y, z);
//	DBG(printbdd_one(wcout<<"rule::get_varbdd"<<endl, y);)
	for (n = vars_arity; n != ar; ++n) from_int_and(pad, bits, n*bits, x);
//	DBG(printbdd_one(wcout<<"rule::get_varbdd"<<endl, bdd_and(x, y));)
	return bdd_and(x, y);
}

size_t std::hash<std::pair<size_t, bools>>::operator()(
	const std::pair<size_t, bools>& m) const {
	std::hash<size_t> h1;
	std::hash<bools> h2;
	return h1(m.first) + h2(m.second);
}
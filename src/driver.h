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
#ifndef __DRIVER_H__
#define __DRIVER_H__
#include <map>
#include <cmath>
#include <variant>
#ifdef WITH_Z3
#include "z3++.h"
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif
#include "tables.h"
#include "input.h"
#include "dict.h"
#include "output.h"
#include "options.h"


typedef std::map<elem, elem> var_subs;
typedef std::pair<std::set<raw_term>, var_subs> terms_hom;

#define QFACT 0
#define QRULE 1
#define QTERM 2
#define QEQUALS 3
#define QFORALL 4
#define QEXISTS 5
#define QNOT 6
#define QAND 7
#define QALT 8
#define QIMPLIES 9
#define QUNIQUE 10
#define QCOIMPLIES 11

struct prog_data {
	strs_t strs;
//	std::unordered_map<int_t, term> strtrees;
//	std::vector<term> out;
//	matpairs r;
//	matrix goals, tgoals;
	bool bwd = false;
	size_t n = 0;
	size_t start_step = 0;
	size_t elapsed_steps = 0;
	string_t std_input;
};

#ifdef WITH_Z3
/* Provides consistent conversions of TML objects into Z3. */
struct z3_context {
	size_t arith_bit_len;
	size_t universe_bit_len;
	z3::context context;
	z3::solver solver;
	z3::expr_vector head_rename;
	z3::sort bool_sort;
	z3::sort value_sort;
	std::map<rel_info, z3::func_decl> rel_to_decl;
	std::map<elem, z3::expr> var_to_decl;
	std::map<raw_rule, z3::expr> rule_to_decl;
	
	z3_context(size_t arith_bit_len, size_t universe_bit_len);
	z3::func_decl rel_to_z3(const raw_term& rt);
	z3::expr globalHead_to_z3(const int_t pos);
	z3::expr fresh_constant();
	z3::expr arg_to_z3(const elem& el);
	z3::expr z3_head_constraints(const raw_term &head,
		std::map<elem, z3::expr> &body_rename);
	z3::expr term_to_z3(const raw_term &rel);
	z3::expr tree_to_z3(const raw_form_tree &tree, dict_t &dict);
	z3::expr rule_to_z3(const raw_rule &rr, dict_t &dict);
};

#endif

void collect_vars(const raw_rule &rr, std::set<elem> &vars);
void collect_vars(const raw_term &rt, std::set<elem> &vars);
template <class InputIterator>
	void collect_vars(InputIterator first, InputIterator last,
		std::set<elem> &vars);
void collect_free_vars(const std::vector<std::vector<raw_term>> &b,
	std::vector<elem> &bound_vars, std::set<elem> &free_vars);
void collect_free_vars(const raw_rule &rr, std::set<elem> &free_vars);
std::set<elem> collect_free_vars(const raw_rule &rr);
void collect_free_vars(const raw_term &t,
	std::vector<elem> &bound_vars, std::set<elem> &free_vars);
std::set<elem> collect_free_vars(const raw_term &t);
void collect_free_vars(const raw_form_tree &t,
	std::vector<elem> &bound_vars, std::set<elem> &free_vars);
std::set<elem> collect_free_vars(const std::vector<std::vector<raw_term>> &b);
std::set<elem> collect_free_vars(const raw_form_tree &t);
std::function<elem (const elem &)> gen_fresh_var(dict_t &d);
elem gen_id_var(const elem &var);
elem rename_variables(const elem &e, std::map<elem, elem> &renames,
	const std::function<elem (const elem &)> &gen);
void rename_variables(raw_form_tree &t, std::map<elem, elem> &renames,
	const std::function<elem (const elem &)> &gen);

class driver {
	friend struct flat_rules;
	friend struct pattern;
	friend std::ostream& operator<<(std::ostream& os, const driver& d);
	friend std::istream& operator>>(std::istream& is, driver& d);
	dict_t dict;
	std::set<int_t> builtin_rels;//, builtin_symbdds;

	int_t nums = 0, chars = 0, syms = 0;

	std::set<lexeme, lexcmp> strs_extra, rels;
	std::vector<ccs> strs_allocated;
	lexeme get_var_lexeme(int_t i);
	lexeme get_new_var();
	lexeme get_lexeme(ccs w, size_t l = (size_t)-1);
	lexeme get_lexeme(const std::basic_string<char>& s);
	lexeme get_lexeme(const std::basic_string<unsigned char>& s);
	lexeme get_new_rel();
//	std::function<int_t(void)> *fget_new_rel;
//	lexeme get_num_lexeme(int_t i);
//	lexeme get_char_lexeme(char_t i);
//	lexeme get_demand_lexeme(elem e, const ints& i, const bools& b);
	void refresh_vars(raw_term& t, size_t& v, std::map<elem, elem>& m);
	void refresh_vars(raw_prog& p);
	raw_rule refresh_vars(raw_term h, std::vector<std::vector<raw_term>> b);
	std::set<raw_rule> refresh_vars(raw_rule& r);
	std::set<raw_term> get_queries(const raw_prog& p);
	string_t directive_load(const directive& d);
	void directives_load(raw_prog& p, lexeme& trel);
	bool transform(raw_prog& rp, const strs_t& strtrees);
//	std::set<raw_rule> transform_ms(const std::set<raw_rule>& p,
//		const std::set<raw_term>& qs);
//	raw_prog transform_sdt(const raw_prog& p);
	void transform_bin(raw_prog& p);
	void transform_len(raw_term& r, const strs_t& s);
//	raw_prog transform_bwd(raw_prog& p);
	raw_term get_try_pred(const raw_term& x);
	void transform_bwd(const raw_term& h, const std::vector<raw_term>& b,
		std::set<raw_rule>& s);
	void transform_bwd(raw_prog& p);
	void transform_proofs(raw_prog& r, const lexeme& rel);
	void transform_string(const string_t&, raw_prog&, const lexeme &);
	void transform_grammar(raw_prog& r, lexeme rel, size_t len);
#ifdef WITH_EVAL
	bool transform_evals(raw_prog &rp, const directive &drt);
#endif
	bool transform_quotes(raw_prog &rp, const directive &drt);
	bool transform_domains(raw_prog &rp, const directive& drt);
	bool transform_codecs(raw_prog &rp, const directive &drt);
	void transform_state_blocks(raw_prog &rp, std::set<lexeme> guards);
	bool is_limited(const elem &var, const raw_form_tree &t,
		std::set<elem> &wrt, std::map<elem, const raw_form_tree*> &scopes);
	bool is_limited(const elem &var, std::set<elem> &wrt,
		std::map<elem, const raw_form_tree*> &scopes);
	std::optional<elem> all_quantifiers_limited(const raw_form_tree &t,
		std::map<elem, const raw_form_tree*> &scopes);
	std::optional<elem> is_safe(const raw_form_tree &t);
	std::optional<elem> is_safe(const raw_rule &rr);
	std::optional<std::pair<elem, raw_rule>> is_safe(raw_prog rp);
	void flatten_associative(const elem::etype &tp,
		const raw_form_tree &tree, std::vector<const raw_form_tree *> &tms);
	template<typename F> void minimize(raw_rule &rr, const F &f);
	template<typename F>
		raw_form_tree &minimize_aux(const raw_rule &ref_rule,
		const raw_rule &var_rule, raw_form_tree &ref_tree,
		raw_form_tree &var_tree, const F &f, bool ctx_sign = true);
	int_t count_related_rules(const raw_rule &rr1, const raw_prog &rp);
	void step_transform(raw_prog &rp,
		const std::function<void(raw_prog &)> &f);
	void pdatalog_transform(raw_prog &rp,
		const std::function<void(raw_prog &)> &f);
	void recursive_transform(raw_prog &rp,
		const std::function<void(raw_prog &)> &f);
	raw_form_tree expand_term(const raw_term &use, const raw_rule &def);
	void square_root_program(raw_prog &rp);
	void square_program(raw_prog &rp);
	raw_rule freeze_rule(raw_rule rr, std::map<elem, elem> &freeze_map,
		dict_t &d);
	bool cqc(const raw_rule &rr1, const raw_rule &rr2);
	bool cqnc(const raw_rule &rr1, const raw_rule &rr2);
	bool cbc(const raw_rule &rr1, raw_rule rr2, std::set<terms_hom> &homs);
	void eliminate_dead_variables(raw_prog &rp);
	void factor_rules(raw_prog &rp);
#ifdef WITH_Z3
	void qc_z3(raw_prog &rp);
	bool check_qc_z3(const raw_rule &r1, const raw_rule &r2,
		z3_context &ctx);
#endif
	raw_prog read_prog(elem prog);
	elem quote_elem(const elem &e, std::map<elem, elem> &variables,
		dict_t &d);
	elem numeric_quote_elem(const elem &e, std::map<elem, elem> &variables);
	elem quote_term(const raw_term &head, const elem &rel_name,
		const elem &domain_name, raw_prog &rp, std::map<elem, elem> &variables,
		int_t &part_count);
	elem quote_formula(const raw_form_tree &t, const elem &rel_name,
		const elem &domain_name, raw_prog &rp, std::map<elem, elem> &variables,
		int_t &part_count);
	std::vector<elem> quote_rule(const raw_rule &rr, const elem &rel_name,
		const elem &domain_name, raw_prog &rp, int_t &part_count);
	void quote_prog(const raw_prog nrp, const elem &rel_name,
		const elem &domain_name, raw_prog &rp);
	void split_heads(raw_prog &rp);
	raw_term to_dnf(const raw_form_tree &t, raw_prog &rp,
		const std::set<elem> &fv);
	void to_dnf(raw_prog &rp);
	void compute_required_vars(const raw_rule &rr, const terms_hom &hom,
		std::set<elem> &orig_vars);
	raw_term relation_to_term(const rel_info &ri);
	bool transform_grammar(raw_prog &rp);
	void export_outer_quantifiers(raw_prog &rp);
	sprawformtree fix_variables(const elem &fv_rel, const elem &qva,
		const elem &rva, const elem &qvb, const elem &rvb);
	sprawformtree fix_symbols(const elem &fs_rel, const elem &qva,
		const elem &rva);
	template<typename F> void subsume_queries(raw_prog &rp, const F &f);
	elem concat(const elem &rel, std::string suffix);
	lexeme concat(const lexeme &rel, std::string suffix);
	string_t generate_cpp(const elem &e, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	string_t generate_cpp(const raw_term &rt, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	string_t generate_cpp(const raw_form_tree &prft, string_t &prog_constr,
		uint_t &cid, const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	string_t generate_cpp(const raw_rule &rr, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	string_t generate_cpp(const lexeme &lex);
	string_t generate_cpp(const directive &dir, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	string_t generate_cpp(const raw_prog &rp, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	raw_prog reify(const raw_prog& p);
	raw_term from_grammar_elem(const elem& v, int_t v1, int_t v2);
	raw_term from_grammar_elem_nt(const lexeme& r, const elem& c,
		int_t v1, int_t v2);
	raw_term from_grammar_elem_builtin(const lexeme& r, const string_t&b,
		int_t v);
	raw_term prepend_arg(const raw_term& t, lexeme s);

	prog_data pd;
	std::set<lexeme> transformed_strings;
	tables *tbl = 0;
	ir_builder *ir = 0;

	std::set<lexeme> vars;
	options opts;
	raw_progs rp;
	bool running = false;
	inputs* ii;
	inputs dynii; // For inputs generated from running TML programs
	input* current_input = 0;
	size_t current_input_id = 0;
	size_t nsteps() { return tbl->step(); };

public:
	bool result = false;
	bool error = false;
	driver(const options& o);
	driver(FILE *f, const options& o);
	driver(string_t, const options& o);
	driver(std::string, const options& o);
	driver(const char *s, const options& o);
	driver(ccs s, const options& o);
	driver(FILE *f);
	driver(std::string);
	driver(string_t);
	driver(const char *s);
	driver(ccs s);
	~driver();

	void read_inputs();
	bool add(input* in);
	void restart();
	bool step(size_t steps = 1, size_t br_on_step=0);
	bool run(size_t steps = 0, size_t br_on_step=0);

	void set_print_step   (bool val) { tbl->print_steps   = val; }
	void set_print_updates(bool val) { tbl->print_updates = val; }
	void set_populate_tml_update(bool val) { tbl->populate_tml_update=val; }
	void set_regex_level(int val ) { ir->regex_level = val; }

	inputs* get_inputs() const { return ii; }
	input* get_current_input() const { return current_input; }
	void set_current_input(input* in) { current_input = in; }

	template <typename T>
	void info(std::basic_ostream<T>&);
	template <typename T>
	void list(std::basic_ostream<T>& os, size_t p = 0);
	template <typename T>
	void out(std::basic_ostream<T>& os) const { if (tbl) tbl->out(os); }
	template <typename T>
	void out_fixpoint(std::basic_ostream<T>& os) const {
		if (tbl) tbl->out_fixpoint(os); }
	template <typename T>
	bool out_goals(std::basic_ostream<T>& os) const {
		return tbl->get_goals(os); }
	template <typename T>
	void out_dict(std::basic_ostream<T>& os) const { tbl->print_dict(os); }

	void dump() { out(o::dump()); }
	void dump_fixpoint() { out_fixpoint(o::dump()); }
	void out(const tables::rt_printer& p) const { if (tbl) tbl->out(p); }

	void save_csv() const;
	
#ifdef __EMSCRIPTEN__
	void out(emscripten::val o) const { if (tbl) tbl->out(o); }
	emscripten::val to_bin() {
		std::stringstream ss; ss << (*this);
		std::string bin = ss.str();
		emscripten::val r = emscripten::val::global("Uint8Array")
							.new_(bin.size());
		int n = 0;
		for (unsigned char b : bin) r.set(n++, b);
		return r;
	}
#endif

};

template void driver::out<char>(std::ostream&) const;
template void driver::out<wchar_t>(std::wostream&) const;
template bool driver::out_goals(std::basic_ostream<char>&) const;
template bool driver::out_goals(std::basic_ostream<wchar_t>&) const;
template void driver::out_dict(std::basic_ostream<char>&) const;
template void driver::out_dict(std::basic_ostream<wchar_t>&) const;

#ifdef DEBUG
extern driver* drv;
//template <typename T>
//std::basic_ostream<T>& printdb(std::basic_ostream<T>& os, lp *p);
template <typename T>
std::basic_ostream<T>& printbdd(std::basic_ostream<T>& os, cr_spbdd_handle t,
	size_t bits, ints ar, int_t rel);
template <typename T>
std::basic_ostream<T>& printbdd_one(std::basic_ostream<T>&os, cr_spbdd_handle t,
	size_t bits, ints ar, int_t rel);
//template <typename T>
//std::basic_ostream<T>& printbdd(std::basic_ostream<T>& os, size_t t, size_t bits, ints ar,
//	int_t rel);
#endif

#endif

/*
 * MathSAT.h
 *
 *  Created on: Jul 17, 2013
 *      Author: Sam Kolton
 */

#ifndef MATHSAT_H_
#define MATHSAT_H_

#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <mathsat/mathsat.h>

namespace mathsat{

class Env;
class Sort;
class Decl;
class Config;
class Expr;
class Solver;

class Env {
private:
	msat_env env_;

public:
	Env(const msat_env& env) : env_(env) {}
	Env(const Config& config);

	~Env() { msat_destroy_env(env_); }

	operator msat_env() const { return env_; }

	void reset();

	Sort bool_sort();
	Sort int_sort();
	Sort rat_sort();
	Sort bv_sort(unsigned size);
	Sort array_sort(const Sort& idx, const Sort& elm);
	Sort simple_sort(const std::string& name);

	Expr constant(const std::string& name, const Sort& type);
	Expr bool_const(const std::string& name);
	Expr int_const(const std::string& name);
	Expr rat_const(const std::string& name);
	Expr bv_const(const std::string& name, const unsigned size);

	Expr bool_val(bool b);
	Expr num_val(int i);
	Expr bv_val(int i, unsigned size);

	Decl function(const std::string& name, const std::vector<Sort> params, const Sort& ret);
	Decl function(const std::string& name, const Sort& param, const Sort& ret);
	Decl function(const std::string& name, const Sort& param1, const Sort& param2, const Sort& ret);
	Decl function(const std::string& name, const Sort& param1, const Sort& param2, const Sort& param3,
			const Sort& ret);
	Decl function(const std::string& name, const Sort& param1, const Sort& param2, const Sort& param3,
			const Sort& param4, const Sort& ret);
	Decl function(const std::string& name, const Sort& param1, const Sort& param2, const Sort& param3,
			const Sort& param4, const Sort& param5,const Sort& ret);
};

///////////////////////////////////////////////////////////////////

class Sort{
private:
	msat_type type_;
	Env& env_;
public:
	Sort(const msat_type& type, Env& env) : type_(type), env_(env) {}

	operator msat_type() const { return type_; }

	bool is_bool() const { return msat_is_bool_type(env_, type_); }
	bool is_int () const { return msat_is_integer_type(env_, type_); }
	bool is_rat() const { return msat_is_rational_type(env_, type_); }
	bool is_arith() const { return is_int() || is_rat(); }
	bool is_bv() const { return msat_is_bv_type(env_, type_, nullptr); }
	bool is_array() const { return msat_is_array_type(env_, type_, nullptr, nullptr); }
	bool equals(const Sort& that) const { return msat_type_equals(type_, that.type_); }


	unsigned bv_size() const;
};

/////////////////////////////////////////////////////////////////

class Decl {
private:
	msat_decl decl_;
	Env& env_;
public:
	Decl(const msat_decl& decl, Env& env) : decl_(decl), env_(env) {}

	operator msat_decl() const { return decl_; }

	unsigned arity() const { return msat_decl_get_arity(decl_); }
	Sort domain(unsigned i) const { return Sort(msat_decl_get_arg_type(decl_, i), env_); }
	Sort range() const { return Sort(msat_decl_get_return_type(decl_), env_); }
	std::string name() const { return msat_decl_get_name(decl_); }

	Expr operator ()(const std::vector<Expr> arg);
	Expr operator ()(const Expr& a);
	Expr operator ()(const Expr& a1, const Expr& a2);
	Expr operator ()(const Expr& a1, const Expr& a2, const Expr& a3);
	Expr operator ()(const Expr& a1, const Expr& a2, const Expr& a3, const Expr& a4);
	Expr operator ()(const Expr& a1, const Expr& a2, const Expr& a3, const Expr& a4, const Expr& a5);

};

//////////////////////////////////////////////////////////////////

class Config {
private:
	msat_config config_;
public:
	Config(const msat_config& config) : config_(config) {}
	Config() { config_ = msat_create_config(); }
	Config(const std::string& logic);
	Config(FILE *f);

	~Config() { msat_destroy_config(config_); }

	operator msat_config() const { return config_; }

	void set(const std::string& option, char const * value) { msat_set_option(config_, option.c_str(), value); }
	void set(const std::string& option, bool value) { msat_set_option(config_, option.c_str(), value ? "true" : "false"); }
	void set(const std::string& option, int value) {
		std::ostringstream oss;
		oss << value;
		msat_set_option(config_, option.c_str(), oss.str().c_str());
	}

	Env env() const { return Env(*this); }
};


//////////////////////////////////////////////////////////////////

using visit_function = msat_visit_status(*)(msat_env, msat_term, int, void *);

#include <iostream>

class Expr {
private:
	Env& env_;
	msat_term term_;

	Sort get_type() const { return Sort(msat_term_get_type(term_), env_); }

public:
	Expr(Env &env, const msat_term& term) : env_(env), term_(term) {}
	Expr(const Expr& that) : env_(that.env_), term_(that.term_) {}

	operator msat_term() const { return term_; }
	Env& env() const { return env_; }

	bool is_bool() const { return msat_is_bool_type(env_, get_type()); }
	bool is_int() const { return msat_is_integer_type(env_, get_type()); }
	bool is_rat() const { return msat_is_rational_type(env_, get_type()); }
	bool is_bv() const { std::cout << "hi" << std::endl; return msat_is_bv_type(env_, get_type(), nullptr); }
	bool is_array() const { return msat_is_array_type(env_, get_type(), nullptr, nullptr); }
	bool is_float() const { return msat_is_fp_type(env_, get_type(), nullptr, nullptr); }
	bool is_float_round() const { return msat_is_fp_roundingmode_type(env_, get_type()); }

	bool type_equals(const Expr &that) const { return msat_type_equals(get_type(), that.get_type()); }

	Decl decl() const;
	unsigned num_args() const { return msat_decl_get_arity(decl()); }
	Sort arg_type(unsigned i) const;

	void visit(visit_function func, void* data);

	friend Expr operator !(Expr const &that);
	friend Expr operator &&(Expr const &a, Expr const &b);
	friend Expr operator &&(Expr const &a, bool b);
	friend Expr operator &&(bool a, Expr const &b);
	friend Expr operator ||(Expr const &a, Expr const &b);
	friend Expr operator ||(Expr const &a, bool b);
	friend Expr operator ||(bool a, Expr const &b);
	friend Expr operator ==(Expr const &a, Expr const &b);
	friend Expr operator ==(Expr const &a, int b);
	friend Expr operator ==(int a, Expr const &b);
	friend Expr operator +(Expr const &a, Expr const &b);
	friend Expr operator +(Expr const &a, int b);
	friend Expr operator +(int a, Expr const &b);
	friend Expr operator *(Expr const &a, Expr const &b);
	friend Expr operator *(Expr const &a, int b);
	friend Expr operator *(int a, Expr const &b);
	friend Expr operator /(Expr const &a, Expr const &b);
	friend Expr operator /(Expr const &a, int b);
	friend Expr operator /(int a, Expr const &b);
	friend Expr operator -(Expr const &a);
	friend Expr operator -(Expr const &a, Expr const &b);
	friend Expr operator -(Expr const &a, int b);
	friend Expr operator -(int a, Expr const &b);
	friend Expr operator <=(Expr const &a, Expr const &b);
	friend Expr operator <=(Expr const &a, int b);
	friend Expr operator <=(int a, Expr const &b);
	friend Expr operator >=(Expr const &a, Expr const &b);
	friend Expr operator >=(Expr const &a, int b);
	friend Expr operator >=(int a, Expr const &b);
	friend Expr operator <(Expr const &a, Expr const &b);
	friend Expr operator <(Expr const &a, int b);
	friend Expr operator <(int a, Expr const &b);
	friend Expr operator >(Expr const &a, Expr const &b);
	friend Expr operator >(Expr const &a, int b);
	friend Expr operator >(int a, Expr const &b);
	friend Expr operator &(Expr const &a, Expr const &b);
	friend Expr operator &(Expr const &a, int b);
	friend Expr operator &(int a, Expr const &b);
	friend Expr operator ^(Expr const &a, Expr const &b);
	friend Expr operator ^(Expr const &a, int b);
	friend Expr operator ^(int a, Expr const &b);
	friend Expr operator |(Expr const &a, Expr const &b);
	friend Expr operator |(Expr const &a, int b);
	friend Expr operator |(int a, Expr const &b);
	friend Expr operator ~(Expr const &a);

	friend Expr iff(Expr const &a, Expr const &b);
	friend Expr ite(const Expr& cond, const Expr& then_, const Expr& else_);
	friend Expr concat(const Expr &a, const Expr &b);
	friend Expr extract(const Expr &a, unsigned msb, unsigned lsb);
	friend Expr lshl(const Expr &a, const Expr &b);
	friend Expr lshr(const Expr &a, const Expr &b);
	friend Expr ashr(const Expr &a, const Expr &b);
	friend Expr udiv(const Expr &a, const Expr &b);
	friend Expr udiv(const Expr &a, const int b);
	friend Expr udiv(const int a, const Expr &b);
	friend Expr comp(const Expr &a, const Expr &b);
	friend Expr rol(const Expr &a, unsigned b);
	friend Expr ror(const Expr &a, unsigned b);
	friend Expr ule(const Expr &a, const Expr &b);
	friend Expr ule(const Expr &a, const int b);
	friend Expr ule(const int a, const Expr &b);
	friend Expr ult(const Expr &a, const Expr &b);
	friend Expr ult(const Expr &a, const int b);
	friend Expr ult(const int a, const Expr &b);
	friend Expr uge(const Expr &a, const Expr &b);
	friend Expr uge(const Expr &a, const int b);
	friend Expr uge(const int a, const Expr &b);
	friend Expr ugt(const Expr &a, const Expr &b);
	friend Expr ugt(const Expr &a, const int b);
	friend Expr ugt(const int a, const Expr &b);

	friend std::ostream& operator <<(std::ostream &out, const Expr& e) {
		char *smtlib = msat_to_smtlib2(e.env_, e.term_);
		out << smtlib;
		msat_free(smtlib);
		return out;
	}

	static Expr from_string(Env& env, std::string data);
	static Expr from_smtlib1(Env& env, std::string data);
	static Expr from_smtlib2(Env& env, std::string data);
}; // class Expr

////////////////////////////////////////////////////////////////

class InterpolationGroup {
private:
	int id_;
public:
	InterpolationGroup(int id) : id_(id) {}

	int id() { return id_; }
};

////////////////////////////////////////////////////////////////

class Solver {
private:
	Env& env_;

public:
	explicit Solver(Env& env) : env_(env) {}

	void add(const Expr& e);

	void push();
	void pop();
	unsigned num_backtrack() { return msat_num_backtrack_points(env_); }

	msat_result check() { return msat_solve(env_); }
	msat_result check(unsigned i, const Expr* assump);

	std::vector<Expr> assertions();
	std::vector<Expr> unsat_core();

	InterpolationGroup create_interp_group() { return msat_create_itp_group(env_); }
	void set_interp_group(InterpolationGroup gr);
	Expr get_interpolant(InterpolationGroup* groupsA, unsigned size);
};


} // namespace mathsat

#endif /* MATHSAT_H_ */

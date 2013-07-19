/*
 * MathSAT.h
 *
 *  Created on: Jul 17, 2013
 *      Author: Sam Kolton
 */

#ifndef MATHSAT_H_
#define MATHSAT_H_

#include <sstream>
#include <mathsat/mathsat.h>

namespace mathsat{

class Decl {
private:
	msat_decl decl_;
public:
	Decl(const msat_decl& decl) : decl_(decl) {}

	operator msat_decl() const { return decl_; }
};

//////////////////////////////////////////////////////////////////

class Sort {
private:
	msat_type type_;
public:
	Sort(const msat_type& type) : type_(type) {}

	operator msat_type() const { return type_; }

};

//////////////////////////////////////////////////////////////////

class Config;

class Env {
private:
	msat_env env_;

public:
	Env(const msat_env& env) : env_(env) {}
	Env(const Config& config);

	~Env() { msat_destroy_env(env_); }

	operator msat_env() { return env_; }
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

	void set(char const *option, char const * value) { msat_set_option(config_, option, value); }
	void set(char const *option, bool value) { msat_set_option(config_, option, value ? "true" : "false"); }
	void set(char const *option, int value) {
		std::ostringstream oss;
		oss << value;
		msat_set_option(config_, option, oss.str().c_str());
	}

	Env env() const { return Env(*this); }
};


//////////////////////////////////////////////////////////////////

class Expr {
private:
	msat_env env_;
	msat_term term_;

	Sort get_type() const { return msat_term_get_type(term_); }

public:
	Expr(const msat_env &env, const msat_term& term) : env_(env), term_(term) {}

	operator msat_term() { return term_; }

	bool is_bool() const { return msat_is_bool_type(env_, get_type()); }
	bool is_int() const { return msat_is_integer_type(env_, get_type()); }
	bool is_rat() const { return msat_is_rational_type(env_, get_type()); }
	bool is_bv() const { return msat_is_bv_type(env_, get_type(), nullptr); }
	bool is_array() const { return msat_is_array_type(env_, get_type(), nullptr, nullptr); }
	bool is_float() const { return msat_is_fp_type(env_, get_type(), nullptr, nullptr); }
	bool is_float_round() const { return msat_is_fp_roundingmode_type(env_, get_type()); }

	bool type_equals(const Expr &that) const { return msat_type_equals(get_type(), that.get_type()); }

	Decl decl() const;

	unsigned num_args() const { return msat_decl_get_arity(decl()); }

	Sort arg_type(unsigned i) const;

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
}; // class Expr




} // namespace mathsat


#endif /* MATHSAT_H_ */

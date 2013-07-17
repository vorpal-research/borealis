/*
 * MathSAT.h
 *
 *  Created on: Jul 17, 2013
 *      Author: Sam Kolton
 */

#ifndef MATHSAT_H_
#define MATHSAT_H_

#include <mathsat/mathsat.h>

namespace mathsat{

class Decl {
private:
	msat_decl decl_;
public:
	Decl(msat_decl decl) : decl_(decl) {}

	operator msat_decl() const { return decl_; }
};

class Sort {
private:
	msat_type type_;
public:
	Sort(msat_type type) : type_(type) {}

	operator msat_type() const { return type_; }

}

class Expr {
private:
	msat_env env_;
	msat_term term_;

	Sort get_type() const { return msat_term_get_type(term_); }

public:
	Expr(msat_env &env, msat_term& term) : env_(env), term_(term) {}

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

}; // class Expr




} // namespace mathsat


#endif /* MATHSAT_H_ */

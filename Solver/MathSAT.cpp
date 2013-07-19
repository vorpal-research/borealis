/*
 * MathSAT.cpp
 *
 *  Created on: Jul 17, 2013
 *      Author: Sam Kolton
 */

#include <string>

#include "MathSAT.h"
#include "Util/util.h"
#include "Util/macros.h"

#define ASSERTMSAT(name, arg) 	bool ass = name(arg);\
								ASSERTC(!ass)

#define ASERTMSAT_TERM	ASSERTMSAT(MSAT_ERROR_TERM, new_term)

namespace mathsat {

Env::Env(const Config& config) {
	env_ = msat_create_env(config);
	ASSERTMSAT(MSAT_ERROR_ENV, env_)
}

//////////////////////////////////////////////////////////////////

Config::Config(const std::string& logic) {
	config_ = msat_create_default_config(logic.c_str());
	ASSERTMSAT(MSAT_ERROR_CONFIG, config_);
}

Config::Config(FILE *f) {
	config_ = msat_parse_config_file(f);
	ASSERTMSAT(MSAT_ERROR_CONFIG, config_);
}

//////////////////////////////////////////////////////////////////

Decl Expr::decl() const {
	msat_decl ret = msat_term_get_decl(term_);
	ASSERTMSAT(MSAT_ERROR_DECL, ret)
	return ret;
}

Sort Expr::arg_type(unsigned i) const {
	ASSERTC(num_args() < i)
	auto type = msat_decl_get_arg_type(decl(), i);
	return type;
}

Expr operator !(Expr const &that){
	ASSERTC(that.is_bool())
	auto new_term = msat_make_not(that.env_, that.term_);
	ASERTMSAT_TERM
	return Expr(that.env_, new_term);
}

Expr operator &&(Expr const &a, Expr const &b){
	ASSERTC(a.is_bool()&& b.is_bool())
	auto new_term = msat_make_and(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator &&(Expr const &a, bool b){
	ASSERTC(a.is_bool())
	auto bool_term = b ? msat_make_true(a.env_) : msat_make_false(a.env_);
	auto new_term = msat_make_and(a.env_, a.term_, bool_term);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator &&(bool a, Expr const &b){
	ASSERTC(b.is_bool())
	auto bool_term = a ? msat_make_true(b.env_) : msat_make_false(b.env_);
	auto new_term = msat_make_and(b.env_, b.term_, bool_term);
	ASERTMSAT_TERM
	return Expr(b.env_, new_term);
}

Expr operator ||(Expr const &a, Expr const &b){
	ASSERTC(a.is_bool()&& b.is_bool())
	auto new_term = msat_make_or(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator ||(Expr const &a, bool b){
	ASSERTC(a.is_bool())
	auto bool_term = b ? msat_make_true(a.env_) : msat_make_false(a.env_);
	auto new_term = msat_make_or(a.env_, a.term_, bool_term);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator ||(bool a, Expr const &b){
	ASSERTC(b.is_bool())
	auto bool_term = a ? msat_make_true(b.env_) : msat_make_false(b.env_);
	auto new_term = msat_make_or(b.env_, b.term_, bool_term);
	ASERTMSAT_TERM
	return Expr(b.env_, new_term);
}

Expr operator ==(Expr const &a, Expr const &b) {
	auto new_term = msat_make_equal(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator ==(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	auto int_term = msat_make_number(a.env_, int_str.c_str());
	auto new_term = msat_make_equal(a.env_, a.term_, int_term);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator ==(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	auto int_term = msat_make_number(b.env_, int_str.c_str());
	auto new_term = msat_make_equal(b.env_, b.term_, int_term);
	ASERTMSAT_TERM
	return Expr(b.env_, new_term);
}

Expr operator +(Expr const &a, Expr const &b) {
	msat_term new_term;
	if ((a.is_rat() || a.is_int() ) && (b.is_rat() || b.is_int())) {
		new_term = msat_make_plus(a.env_, a.term_, b.term_);
	}
	else if (a.is_bv() && b.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		ASSERTC(a_width == b_width)
		new_term = msat_make_bv_plus(a.env_, a.term_, b.term_);
	}
	else {
		ASSERTC(false)
	}
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator +(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	msat_term int_term;
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	} else {
		int_term = msat_make_number(a.env_, int_str.c_str());
	}
	return a + Expr(a.env_, int_term);
}

Expr operator +(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	msat_term int_term;
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	} else {
		int_term = msat_make_number(b.env_, int_str.c_str());
	}
	return Expr(b.env_, int_term) + b;
}

Expr operator *(Expr const &a, Expr const &b) {
	msat_term new_term;
	if ((a.is_rat() || a.is_int() ) && (b.is_rat() || b.is_int())) {
		new_term = msat_make_times(a.env_, a.term_, b.term_);
	}
	else if (a.is_bv() && b.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		ASSERTC(a_width == b_width)
		new_term = msat_make_bv_times(a.env_, a.term_, b.term_);
	}
	else {
		ASSERTC(false)
	}
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator *(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	msat_term int_term;
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	} else {
		int_term = msat_make_number(a.env_, int_str.c_str());
	}
	return a * Expr(a.env_, int_term);
}

Expr operator *(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	msat_term int_term;
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	} else {
		int_term = msat_make_number(b.env_, int_str.c_str());
	}
	return Expr(b.env_, int_term) * b;
}

Expr operator /(Expr const &a, Expr const &b) {
	ASSERTC( a.is_bv() && b.is_bv())
	msat_term new_term = msat_make_bv_sdiv(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator /(Expr const &a, int b){
	ASSERTC( a.is_bv())
	std::string int_str = std::to_string(b);
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	msat_term int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	return a / Expr(a.env_, int_term);
}

Expr operator /(int a, Expr const &b){
	ASSERTC( b.is_bv())
	std::string int_str = std::to_string(a);
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	msat_term int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	return Expr(b.env_, int_term) / b;
}

Expr operator -(Expr const &a) {
	ASSERTC( a.is_bv() )
	msat_term new_term = msat_make_bv_neg(a.env_, a.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator -(Expr const &a, Expr const &b) {
	ASSERTC( a.is_bv() && b.is_bv())
	msat_term new_term = msat_make_bv_minus(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator -(Expr const &a, int b){
	ASSERTC( a.is_bv())
	std::string int_str = std::to_string(b);
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	msat_term int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	return a - Expr(a.env_, int_term);
}

Expr operator -(int a, Expr const &b){
	ASSERTC( b.is_bv())
	std::string int_str = std::to_string(a);
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	msat_term int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	return Expr(b.env_, int_term) - b;
}

Expr operator <=(Expr const &a, Expr const &b) {
	msat_term new_term;
	if ((a.is_rat() || a.is_int() ) && (b.is_rat() || b.is_int())) {
		new_term = msat_make_leq(a.env_, a.term_, b.term_);
	}
	else if (a.is_bv() && b.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		ASSERTC(a_width == b_width)
		new_term = msat_make_bv_sleq(a.env_, a.term_, b.term_);
	}
	else {
		ASSERTC(false)
	}
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator <=(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	msat_term int_term;
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	} else {
		int_term = msat_make_number(a.env_, int_str.c_str());
	}
	return a <= Expr(a.env_, int_term);
}

Expr operator <=(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	msat_term int_term;
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	} else {
		int_term = msat_make_number(b.env_, int_str.c_str());
	}
	return Expr(b.env_, int_term) <= b;
}

Expr operator >=(Expr const &a, Expr const &b) {
	msat_term new_term;
	if ((a.is_rat() || a.is_int() ) && (b.is_rat() || b.is_int())) {
		new_term = msat_make_leq(a.env_, a.term_, b.term_);
		new_term = msat_make_not(a.env_, new_term);
		auto eq_term = msat_make_equal(a.env_, a.term_, b.term_);
		new_term = msat_make_or(a.env_, new_term, eq_term);
	}
	else if (a.is_bv() && b.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		ASSERTC(a_width == b_width)
		new_term = msat_make_bv_sleq(a.env_, a.term_, b.term_);
		new_term = msat_make_bv_not(a.env_, new_term);
		auto eq_term = msat_make_equal(a.env_, a.term_, b.term_);
		new_term = msat_make_bv_or(a.env_, new_term, eq_term);
	}
	else {
		ASSERTC(false)
	}
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator >=(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	msat_term int_term;
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	} else {
		int_term = msat_make_number(a.env_, int_str.c_str());
	}
	return a >= Expr(a.env_, int_term);
}

Expr operator >=(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	msat_term int_term;
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	} else {
		int_term = msat_make_number(b.env_, int_str.c_str());
	}
	return Expr(b.env_, int_term) >= b;
}

Expr operator <(Expr const &a, Expr const &b) {
	msat_term new_term;
	if ((a.is_rat() || a.is_int() ) && (b.is_rat() || b.is_int())) {
		new_term = msat_make_leq(a.env_, a.term_, b.term_);
		auto eq_term = msat_make_equal(a.env_, a.term_, b.term_);
		eq_term = msat_make_not(a.env_, eq_term);
		new_term = msat_make_and(a.env_, new_term, eq_term);
	}
	else if (a.is_bv() && b.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		ASSERTC(a_width == b_width)
		new_term = msat_make_bv_sleq(a.env_, a.term_, b.term_);
		auto eq_term = msat_make_equal(a.env_, a.term_, b.term_);
		eq_term = msat_make_bv_not(a.env_, eq_term);
		new_term = msat_make_bv_and(a.env_, new_term, eq_term);
	}
	else {
		ASSERTC(false)
	}
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator <(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	msat_term int_term;
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	} else {
		int_term = msat_make_number(a.env_, int_str.c_str());
	}
	return a < Expr(a.env_, int_term);
}

Expr operator <(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	msat_term int_term;
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	} else {
		int_term = msat_make_number(b.env_, int_str.c_str());
	}
	return Expr(b.env_, int_term) < b;
}

Expr operator >(Expr const &a, Expr const &b) {
	msat_term new_term;
	if ((a.is_rat() || a.is_int() ) && (b.is_rat() || b.is_int())) {
		new_term = msat_make_leq(a.env_, a.term_, b.term_);
		new_term = msat_make_not(a.env_, new_term);
	}
	else if (a.is_bv() && b.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		ASSERTC(a_width == b_width)
		new_term = msat_make_bv_sleq(a.env_, a.term_, b.term_);
		new_term = msat_make_bv_not(a.env_, new_term);
	}
	else {
		ASSERTC(false)
	}
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator >(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	msat_term int_term;
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	} else {
		int_term = msat_make_number(a.env_, int_str.c_str());
	}
	return a > Expr(a.env_, int_term);
}

Expr operator >(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	msat_term int_term;
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	} else {
		int_term = msat_make_number(b.env_, int_str.c_str());
	}
	return Expr(b.env_, int_term) > b;
}

Expr operator &(Expr const &a, Expr const &b){
	ASSERTC(a.is_bv()&& b.is_bv())
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	ASSERTC(a_width == b_width)
	auto new_term = msat_make_bv_and(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator &(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	auto int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	return a & Expr(a.env_, int_term);
}

Expr operator &(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	auto int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	return Expr(b.env_, int_term) & b;
}

Expr operator ^(Expr const &a, Expr const &b){
	ASSERTC(a.is_bv()&& b.is_bv())
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	ASSERTC(a_width == b_width)
	auto new_term = msat_make_bv_xor(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator ^(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	auto int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	return a ^ Expr(a.env_, int_term);
}

Expr operator ^(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	auto int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	return Expr(b.env_, int_term) ^ b;
}

Expr operator |(Expr const &a, Expr const &b){
	ASSERTC(a.is_bv()&& b.is_bv())
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	ASSERTC(a_width == b_width)
	auto new_term = msat_make_bv_or(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator |(Expr const &a, int b){
	std::string int_str = std::to_string(b);
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	auto int_term = msat_make_bv_number(a.env_, int_str.c_str(), a_width, 10);
	return a | Expr(a.env_, int_term);
}

Expr operator |(int a, Expr const &b){
	std::string int_str = std::to_string(a);
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	auto int_term = msat_make_bv_number(b.env_, int_str.c_str(), b_width, 10);
	return Expr(b.env_, int_term) | b;
}

Expr operator ~(Expr const &that){
	ASSERTC(that.is_bool())
	auto new_term = msat_make_bv_not(that.env_, that.term_);
	ASERTMSAT_TERM
	return Expr(that.env_, new_term);
}

// ToDo sam Write pw() function using msat_make_times()
// ToDo sam Try to realize rat and int div and minus using smtlib.

Expr iff(Expr const &a, Expr const &b){
	ASSERTC(a.is_bool()&& b.is_bool())
	auto new_term = msat_make_iff(a.env_, a.term_, b.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

} // namespace mathsat




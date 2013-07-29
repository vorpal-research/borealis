/*
 * MathSAT.cpp
 *
 *  Created on: Jul 17, 2013
 *      Author: Sam Kolton
 */

#include <functional>

#include "MathSAT.h"

#include "Util/util.h"
#include "Util/macros.h"

#define ASSERTMSAT(name, arg) 	bool ass = name(arg);\
								ASSERTC(!ass)

#define ASSERTMSAT_NB(name, arg) 	ass = name(arg);\
									ASSERTC(!ass)

#define ASERTMSAT_TERM	ASSERTMSAT(MSAT_ERROR_TERM, new_term)

#define ASERTMSAT_DECL	ASSERTMSAT(MSAT_ERROR_DECL, new_decl)


namespace mathsat {


unsigned Sort::bv_size() const {
	ASSERTC(is_bv())
	size_t width;
	msat_is_bv_type(env_, type_, &width);
	return width;
}

/////////////////////////////////////////////////////////////////

Expr Decl::operator ()(const std::vector<Expr> arg) {
	std::vector<msat_term> msat_arg(arg.begin(), arg.end());
	auto new_term = msat_make_term(env_, decl_, msat_arg.data());
	ASERTMSAT_TERM
	return Expr(env_, new_term);
}

Expr Decl::operator ()(const Expr& a) {
	std::vector<Expr> params;
	params.emplace_back(a);
	return (*this)(params);
}

Expr Decl::operator ()(const Expr& a1, const Expr& a2) {
	std::vector<Expr> params;
	params.emplace_back(a1);
	params.emplace_back(a2);
	return (*this)(params);
}

Expr Decl::operator ()(const Expr& a1, const Expr& a2, const Expr& a3) {
	std::vector<Expr> params;
	params.emplace_back(a1);
	params.emplace_back(a2);
	params.emplace_back(a3);
	return (*this)(params);
}

Expr Decl::operator ()(const Expr& a1, const Expr& a2, const Expr& a3, const Expr& a4) {
	std::vector<Expr> params;
	params.emplace_back(a1);
	params.emplace_back(a2);
	params.emplace_back(a3);
	params.emplace_back(a4);
	return (*this)(params);
}

Expr Decl::operator ()(const Expr& a1, const Expr& a2, const Expr& a3, const Expr& a4, const Expr& a5) {
	std::vector<Expr> params;
	params.emplace_back(a1);
	params.emplace_back(a2);
	params.emplace_back(a3);
	params.emplace_back(a4);
	params.emplace_back(a5);
	return (*this)(params);
}


///////////////////////////////////////////////////////////////////

Env::Env(const Config& config) {
	env_ = msat_create_env(config);
	ASSERTMSAT(MSAT_ERROR_ENV, env_)
}

void Env::reset() {
	int res = msat_reset_env(env_);
	ASSERTC(!res)
}

Sort Env::bool_sort() {
	return Sort(msat_get_bool_type(env_), *this);
}
Sort Env::int_sort() {
	return Sort(msat_get_integer_type(env_), *this);
}
Sort Env::rat_sort() {
	//return Sort(msat_get_rational_type(env_), env_);
	return Sort(msat_get_rational_type(env_), *this);
}
Sort Env::bv_sort(unsigned size) {
	return Sort(msat_get_bv_type(env_, size), *this);
}
Sort Env::array_sort(const Sort& idx, const Sort& elm) {
	return Sort(msat_get_array_type(env_, idx, elm), *this);
}
Sort Env::simple_sort(const std::string& name) {
	return Sort(msat_get_simple_type(env_, name.c_str()), *this);
}

Expr Env::constant(const std::string& name, const Sort& type) {
	auto new_decl = msat_declare_function(env_, name.c_str(), type);
	ASERTMSAT_DECL
	auto new_term = msat_make_constant(env_, new_decl);
	ASSERTMSAT_NB(MSAT_ERROR_TERM, new_term)
	return Expr(*this, new_term);
}

Expr Env::bool_const(const std::string& name) {
	return constant(name, bool_sort());
}
Expr Env::int_const(const std::string& name) {
	return constant(name, int_sort());
}
Expr Env::rat_const(const std::string& name) {
	return constant(name, rat_sort());
}
Expr Env::bv_const(const std::string& name, const unsigned size) {
	return constant(name, bv_sort(size));
}

Expr Env::bool_val(bool b) {
	auto new_term = b ? msat_make_true(env_) : msat_make_false(env_);
	ASERTMSAT_TERM
	return Expr(*this, new_term);
}

Expr Env::num_val(int i) {
	std::string int_str = std::to_string(i);
	msat_term new_term = msat_make_number(env_, int_str.c_str());
	ASERTMSAT_TERM
	return Expr(*this, new_term);
}

Expr Env::bv_val(int i, unsigned size) {
	std::string int_str = std::to_string(i);
	msat_term new_term = msat_make_bv_number(env_, int_str.c_str(), size, 10);
	ASERTMSAT_TERM
	return Expr(*this, new_term);
}

Decl Env::function(const std::string& name, const std::vector<Sort> params, const Sort& ret) {
	std::vector<msat_type> msat_param(params.begin(), params.end());
	auto type = msat_get_function_type(env_, msat_param.data(), msat_param.size(), ret);
	Sort func_type = Sort(type, *this);
	msat_decl new_decl = msat_declare_function(env_, name.c_str(), func_type);
	ASERTMSAT_DECL
	return Decl(new_decl, *this);
}

Decl Env::function(const std::string& name, const Sort& param, const Sort& ret) {
	msat_type msat_param[1] = {param};
	auto type = msat_get_function_type(env_, msat_param, 1, ret);
	Sort func_type = Sort(type, *this);
	msat_decl new_decl = msat_declare_function(env_, name.c_str(), func_type);
	ASERTMSAT_DECL
	return Decl(new_decl, *this);
}

Decl Env::function(const std::string& name, const Sort& param1, const Sort& param2, const Sort& ret) {
	std::vector<Sort> params;
	params.push_back(param1);
	params.push_back(param2);
	return function(name, params, ret);
}
Decl Env::function(const std::string& name, const Sort& param1, const Sort& param2, const Sort& param3,
		const Sort& ret) {
	std::vector<Sort> params;
	params.push_back(param1);
	params.push_back(param2);
	params.push_back(param3);
	return function(name, params, ret);
}

Decl Env::function(const std::string& name, const Sort& param1, const Sort& param2, const Sort& param3,
		const Sort& param4, const Sort& ret) {
	std::vector<Sort> params;
	params.push_back(param1);
	params.push_back(param2);
	params.push_back(param3);
	params.push_back(param4);
	return function(name, params, ret);
}

Decl Env::function(const std::string& name, const Sort& param1, const Sort& param2, const Sort& param3,
		const Sort& param4, const Sort& param5,const Sort& ret) {
	std::vector<Sort> params;
	params.push_back(param1);
	params.push_back(param2);
	params.push_back(param3);
	params.push_back(param4);
	params.push_back(param5);
	return function(name, params, ret);
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

void Expr::visit(visit_function func, void* data) {
	int ret = msat_visit_term(env_, term_, func, data);
	ASSERTC(!ret)
}

Decl Expr::decl() const {
	msat_decl ret = msat_term_get_decl(term_);
	ASSERTMSAT(MSAT_ERROR_DECL, ret)
	return Decl(ret, env_);
}

Sort Expr::arg_type(unsigned i) const {
	ASSERTC(num_args() < i)
	auto type = msat_decl_get_arg_type(decl(), i);
	return Sort(type, env_);
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
	Expr bool_term = a.env_.bool_val(b);
	//auto bool_term = b ? msat_make_true(a.env_) : msat_make_false(a.env_);
	auto new_term = msat_make_and(a.env_, a.term_, bool_term);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator &&(bool a, Expr const &b){
	ASSERTC(b.is_bool())
	Expr bool_term = b.env_.bool_val(a);
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
	Expr bool_term = a.env_.bool_val(b);
	auto new_term = msat_make_or(a.env_, a.term_, bool_term);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator ||(bool a, Expr const &b){
	ASSERTC(b.is_bool())
	Expr bool_term = b.env_.bool_val(a);
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
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
		return a + Expr(a.env_, int_term);
	} else {
		Expr int_term = a.env_.num_val(b);
		return a + Expr(a.env_, int_term);
	}
}

Expr operator +(int a, Expr const &b){
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
		return Expr(b.env_, int_term) + b;
	} else {
		Expr int_term = b.env_.num_val(a);
		return Expr(b.env_, int_term) + b;
	}
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
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
		return a * Expr(a.env_, int_term);
	} else {
		Expr int_term = a.env_.num_val(b);
		return a * Expr(a.env_, int_term);
	}
}

Expr operator *(int a, Expr const &b){
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
		return Expr(b.env_, int_term) * b;
	} else {
		Expr int_term = b.env_.num_val(a);
		return Expr(b.env_, int_term) * b;
	}
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

#include <iostream>

Expr operator -(Expr const &a) {
	std::cout << a.is_bv() << std::endl;
	ASSERTC( a.is_bv() )
	std::cout << a.is_bv() << std::endl;
	msat_term new_term = msat_make_bv_neg(a.env_, a.term_);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr operator -(Expr const &a, Expr const &b) {
	std::cout << a.is_bv() << " " << b.is_bv() << std::endl;
	ASSERTC( a.is_bv() && b.is_bv())
	std::cout << a.is_bv() << " " << b.is_bv() << std::endl;
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
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
		return a <= Expr(a.env_, int_term);
	} else {
		Expr int_term = a.env_.num_val(b);
		return a <= Expr(a.env_, int_term);
	}
}

Expr operator <=(int a, Expr const &b){
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
		return Expr(b.env_, int_term) <= b;
	} else {
		Expr int_term = b.env_.num_val(a);
		return Expr(b.env_, int_term) <= b;
	}
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
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
		return a >= Expr(a.env_, int_term);
	} else {
		Expr int_term = a.env_.num_val(b);
		return a >= Expr(a.env_, int_term);
	}
}

Expr operator >=(int a, Expr const &b){
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
		return Expr(b.env_, int_term) >= b;
	} else {
		Expr int_term = b.env_.num_val(a);
		return Expr(b.env_, int_term) >= b;
	}
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
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
		return a < Expr(a.env_, int_term);
	} else {
		Expr int_term = a.env_.num_val(b);
		return a < Expr(a.env_, int_term);
	}
}

Expr operator <(int a, Expr const &b){
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
		return Expr(b.env_, int_term) < b;
	} else {
		Expr int_term = b.env_.num_val(a);
		return Expr(b.env_, int_term) < b;
	}
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
	if (a.is_bv()) {
		size_t a_width;
		msat_is_bv_type(a.env_, a.get_type(), &a_width);
		Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
		return a > Expr(a.env_, int_term);
	} else {
		Expr int_term = a.env_.num_val(b);
		return a > Expr(a.env_, int_term);
	}
}

Expr operator >(int a, Expr const &b){
	if (b.is_bv()) {
		size_t b_width;
		msat_is_bv_type(b.env_, b.get_type(), &b_width);
		Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
		return Expr(b.env_, int_term) > b;
	} else {
		Expr int_term = b.env_.num_val(a);
		return Expr(b.env_, int_term) > b;
	}
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
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
	return a & Expr(a.env_, int_term);
}

Expr operator &(int a, Expr const &b){
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
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
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
	return a ^ Expr(a.env_, int_term);
}

Expr operator ^(int a, Expr const &b){
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
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
	size_t a_width;
	msat_is_bv_type(a.env_, a.get_type(), &a_width);
	Expr int_term = a.env_.bv_val(b, static_cast<unsigned int>(a_width));
	return a | Expr(a.env_, int_term);
}

Expr operator |(int a, Expr const &b){
	size_t b_width;
	msat_is_bv_type(b.env_, b.get_type(), &b_width);
	Expr int_term = b.env_.bv_val(a, static_cast<unsigned int>(b_width));
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

Expr ite(const Expr& cond, const Expr& then_, const Expr& else_) {
	ASSERTC(cond.is_bool())
	auto new_term = msat_make_term_ite(cond.env_, cond, then_, else_);
	ASERTMSAT_TERM
	return Expr(cond.env_, new_term);
}

Expr concat(const Expr &a, const Expr &b) {
	ASSERTC(a.is_bv() && b.is_bv())
	auto new_term = msat_make_bv_concat(a.env_, a, b);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr extract(const Expr &a, unsigned msb, unsigned lsb) {
	ASSERTC(a.is_bv())
	auto new_term = msat_make_bv_extract(a.env_, msb, lsb, a);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

#define BV_FUNC_GENERATOR(name1, name2) 	Expr name1(const Expr &a, const Expr &b) { \
												ASSERTC(a.is_bv() && b.is_bv())	\
												ASSERTC(a.get_type().bv_size() == b.get_type().bv_size())	\
												auto new_term = name2(a.env_, a, b);	\
												ASERTMSAT_TERM	\
												return Expr(a.env_, new_term);	\
											}

BV_FUNC_GENERATOR(lshl, msat_make_bv_lshl)
BV_FUNC_GENERATOR(lshr, msat_make_bv_lshr)
BV_FUNC_GENERATOR(ashr, msat_make_bv_ashr)
BV_FUNC_GENERATOR(udiv, msat_make_bv_udiv)
BV_FUNC_GENERATOR(comp, msat_make_bv_comp)

Expr udiv(const Expr &a, const int b) {
	auto int_term = a.env_.num_val(b);
	return udiv(a, int_term);
}

Expr udiv(const int a, const Expr &b) {
	auto int_term = b.env_.num_val(a);
	return udiv(int_term, b);
}

Expr rol(const Expr &a, unsigned b) {
	ASSERTC(a.is_bv())
	auto new_term = msat_make_bv_rol(a.env_, b, a);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr ror(const Expr &a, unsigned b) {
	ASSERTC(a.is_bv())
	auto new_term = msat_make_bv_ror(a.env_, b, a);
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

BV_FUNC_GENERATOR(ule, msat_make_bv_uleq)
BV_FUNC_GENERATOR(ult, msat_make_bv_ult)

Expr ule(const Expr &a, const int b) {
	auto int_term = a.env_.num_val(b);
	return ule(a, int_term);
}

Expr ule(const int a, const Expr &b) {
	auto int_term = b.env_.num_val(a);
	return ule(b, int_term);
}

Expr ult(const Expr &a, const int b) {
	auto int_term = a.env_.num_val(b);
	return ult(a, int_term);
}

Expr ult(const int a, const Expr &b) {
	auto int_term = b.env_.num_val(a);
	return ult(b, int_term);
}

Expr uge(const Expr &a, const Expr &b) {
	ASSERTC(a.is_bv() && b.is_bv())
	ASSERTC(a.get_type().bv_size() == b.get_type().bv_size())
	auto new_term = msat_make_bv_not(a.env_, msat_make_bv_ult(a.env_, a, b));
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr uge(const Expr &a, const int b) {
	auto int_term = a.env_.num_val(b);
	return uge(a, int_term);
}

Expr uge(const int a, const Expr &b) {
	auto int_term = b.env_.num_val(a);
	return uge(b, int_term);
}

Expr ugt(const Expr &a, const Expr &b) {
	ASSERTC(a.is_bv() && b.is_bv())
	ASSERTC(a.get_type().bv_size() == b.get_type().bv_size())
	auto new_term = msat_make_bv_not(a.env_, msat_make_bv_uleq(a.env_, a, b));
	ASERTMSAT_TERM
	return Expr(a.env_, new_term);
}

Expr ugt(const Expr &a, const int b) {
	auto int_term = a.env_.num_val(b);
	return ugt(a, int_term);
}

Expr ugt(const int a, const Expr &b) {
	auto int_term = b.env_.num_val(a);
	return ugt(b, int_term);
}

#undef BV_FUNC_GENERATOR



Expr Expr::from_string(Env& env, std::string data) {
	auto new_term = msat_from_string(env, data.c_str());
	ASERTMSAT_TERM
	return Expr(env, new_term);
}

Expr Expr::from_smtlib1(Env& env, std::string data) {
	auto new_term = msat_from_smtlib1(env, data.c_str());
	ASERTMSAT_TERM
	return Expr(env, new_term);
}

Expr Expr::from_smtlib2(Env& env, std::string data) {
	auto new_term = msat_from_smtlib2(env, data.c_str());
	ASERTMSAT_TERM
	return Expr(env, new_term);
}


///////////////////////////////////////////////////////////////////////

void Solver::add(const Expr& e) {
	int res = msat_assert_formula(env_, e);
	ASSERTC(!res)
}

void Solver::push() {
	int res = msat_push_backtrack_point(env_);
	ASSERTC(!res)
}
void Solver::pop() {
	int res = msat_pop_backtrack_point(env_);
	ASSERTC(!res)
}

msat_result Solver::check(unsigned i, const Expr* assump) {
	msat_term msat_assump[i];
	for (unsigned idx = 0; idx < i; idx++) {
		msat_assump[idx] = assump[idx];
	}
	return msat_solve_with_assumptions(env_, msat_assump, i);
}

std::vector<Expr> Solver::assertions() {
	size_t size;
	msat_term* msat_exprs = msat_get_asserted_formulas(env_, &size);
	std::vector<Expr> exprs;
	for (unsigned idx = 0; idx < size; idx++) {
		exprs.emplace_back(env_, msat_exprs[idx]);
	}
	msat_free(msat_exprs);
	return exprs;
}

std::vector<Expr> Solver::unsat_core() {
	size_t size;
	msat_term* msat_exprs = msat_get_unsat_core(env_, &size);
	std::vector<Expr> exprs;
	for (unsigned idx = 0; idx < size; idx++) {
		exprs.emplace_back(env_, msat_exprs[idx]);
	}
	msat_free(msat_exprs);
	return exprs;
}

void Solver::set_interp_group(InterpolationGroup gr) {
	int res = msat_set_itp_group(env_, gr.id());
	ASSERTC(!res)
}

Expr Solver::get_interpolant(InterpolationGroup* groupsA, unsigned size) {
	int idxs[size];
	for (unsigned i=0; i < size; i++) {
		idxs[i] = groupsA[i].id();
	}
	auto new_term = msat_get_interpolant(env_, idxs, size);
	ASERTMSAT_TERM
	return Expr(env_, new_term);
}

} // namespace mathsat

#include "Util/unmacros.h"


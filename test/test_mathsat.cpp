/*
 * test_mathsat.cpp
 *
 *  Created on: Jul 29, 2013
 *      Author: Sam Kolton
 */

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <iostream>
#include <mathsat/mathsat.h>
#include "SMT/MathSAT/MathSAT.h"

namespace{

using namespace mathsat;
using std::string;

TEST(MathSatApi, NotBitVector) {
	Config conf = Config();
	Env env = Env(conf);
	Expr x = env.int_const("x");
	Expr y = env.int_const("y");
	ASSERT_TRUE(x.is_int());
	ASSERT_TRUE(y.is_int());
	ASSERT_FALSE(x.is_bv());
	ASSERT_FALSE(y.is_bv());
	std::cout << "asdhkfgbalejwfb" << std::endl;
//	Expr x = env.bv_const("x", 32);
//	Expr y = env.bv_const("y", 32);
	Expr z = -y;

}

TEST(MathSatApi, generatingFormulas) {
	// Comparing internal representations of two identical formulas:
	// first created using C++ API
	// second created using standard C API

	// getting term representation using C++ API
	Config conf = Config();
	Env env = Env(conf);

	Sort rat = env.rat_sort();
	std::vector<Sort> empty_params;
	Decl f = env.function("f", rat, rat);
	Expr x1 = env.rat_const("x1");
	Expr x2 = env.rat_const("x2");
	Expr x3 = env.rat_const("x3");
	Expr y1 = env.rat_const("y1");
	Expr y2 = env.rat_const("y2");
	Expr y3 = env.rat_const("y3");

	Expr A = (x2 + f(x1) - x3 == 0) && (y2 + f(y1) == y3) && (y1 <= x1);
	char *msat_str = msat_term_repr(A);
	string cppterm = msat_str;
	msat_free(msat_str);


	// getting term representation using standard C API
	msat_config cfg;
	msat_env menv;
	msat_term formula;
	const char *vars[] = {"x1", "x2", "x3", "y1", "y2", "y3"};
	const char *ufs = "f";
	unsigned int i;
	msat_type rat_tp, func_tp;
	msat_type paramtps[1];

	cfg = msat_create_config();
	menv = msat_create_env(cfg);
	msat_destroy_config(cfg);

	rat_tp = msat_get_rational_type(menv);
	paramtps[0] = rat_tp;
	func_tp = msat_get_function_type(menv, paramtps, 1, rat_tp);

	for (i = 0; i < sizeof(vars)/sizeof(vars[0]); ++i) {
		msat_declare_function(menv, vars[i], rat_tp);
	}
	msat_declare_function(menv, ufs, func_tp);
	formula = msat_from_string(menv, "(and (and (= (+ x2 (f x1)) x3) (= (+ (f y1) y2) y3)) (<= y1 x1))");
	msat_str = msat_term_repr(formula);
	string cterm = msat_str;
	msat_free(msat_str);
	msat_destroy_env(menv);

	ASSERT_EQ(cppterm, cterm);
}

}





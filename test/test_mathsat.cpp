/*
 * test_mathsat.cpp
 *
 *  Created on: Jul 29, 2013
 *      Author: Sam Kolton
 */

#include <gtest/gtest.h>
#include <mathsat/mathsat.h>

#include <iostream>
#include <string>
#include <vector>

#include "SMT/MathSAT/MathSAT.h"

namespace {

using namespace borealis::mathsat;

TEST(MathSatApi, generatingFormulas) {
	// Comparing two identical formulas:
	// - first one created using our C++ API
	// - second one created using standard C API

	// C++ API
	Config conf = Config();
	Env env = Env(conf);

	Sort rat = env.rat_sort();
	Decl f = env.function("f", {rat}, rat);
	Expr x1 = env.rat_const("x1");
	Expr x2 = env.rat_const("x2");
	Expr x3 = env.rat_const("x3");
	Expr y1 = env.rat_const("y1");
	Expr y2 = env.rat_const("y2");
	Expr y3 = env.rat_const("y3");

	Expr A = (f(x1) - x2 == -x3 ) &&
	         (f(y1) + y2 == y3) &&
	         (y1 <= x1);

	// C API
	msat_term formula;
	formula = msat_from_string(env,
        "(and (= (- (f x1) x2) (- x3))"
        "(= (+ (f y1) y2) y3)"
        "(<= y1 x1))"
    );

	Solver sol = Solver(env);
	sol.add(A);
	msat_assert_formula(env, msat_make_not(env, formula));

    msat_result res = sol.check();
	ASSERT_EQ(res, MSAT_UNSAT);
}

TEST(MathSatApi, generatingInterpolant) {
	// Comparing two interpolants for same formulas:
	// - first one generated using our C++ API
	// - second one generated using standard C API

	// C++ API
	Config conf = Config();
	conf.set("interpolation", true);

	Env env = Env(conf);

	Sort rat = env.rat_sort();
	Decl f = env.function("f", {rat}, rat);
	Decl g = env.function("g", {rat}, rat);
	Expr x1 = env.rat_const("x1");
	Expr x2 = env.rat_const("x2");
	Expr x3 = env.rat_const("x3");
	Expr y1 = env.rat_const("y1");
	Expr y2 = env.rat_const("y2");
	Expr y3 = env.rat_const("y3");
	Expr b = env.rat_const("b");

	Expr A = (f(x1) + x2 == x3) &&
	         (f(y1) + y2 == y3) &&
	         (y1 <= x1);
	Expr B = (g(b) == x2) &&
	         (g(b) == y2) &&
	         (x1 <= y1) &&
	         (x3 < y3);

	Solver sol = Solver(env);
	sol.push();
	Solver::InterpolationGroup grA = sol.create_interp_group();
	Solver::InterpolationGroup grB = sol.create_interp_group();
	sol.set_interp_group(grA);
	sol.add(A);
	sol.set_interp_group(grB);
	sol.add(B);

	auto res = sol.check();
	ASSERT_EQ(res, MSAT_UNSAT);
	Expr cppInterp = sol.get_interpolant({grA});
	sol.pop();

	// C API
	msat_term formula;
	msat_push_backtrack_point(env);
	int group_a = msat_create_itp_group(env);
	int group_b = msat_create_itp_group(env);

	formula = msat_from_string(env,
        "(and (= (+ (f x1) x2) x3)"
        "(= (+ (f y1) y2) y3)"
        "(<= y1 x1))"
    );
	msat_set_itp_group(env, group_a);
	msat_assert_formula(env, formula);

	formula = msat_from_string(env,
        "(and (= x2 (g b))"
	    "(= y2 (g b))"
        "(<= x1 y1)"
        "(< x3 y3))"
    );
	msat_set_itp_group(env, group_b);
	msat_assert_formula(env, formula);

	res = msat_solve(env);
	ASSERT_EQ(res, MSAT_UNSAT);
	msat_term cInterp = msat_get_interpolant(env, &group_a, 1);
	msat_pop_backtrack_point(env);

	// Comparing interpolants
	sol.add(cppInterp);
	int r = msat_assert_formula(env, msat_make_not(env, cInterp));
	ASSERT_EQ(r, 0);

	res = sol.check();
	ASSERT_EQ(res, MSAT_UNSAT);
}

} // namespace

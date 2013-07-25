/*
 * test.cpp
 *
 *  Created on: Jul 24, 2013
 *      Author: sam
 */

#include "MathSAT.h"
#include "test.h"
#include <iostream>
#include <mathsat/mathsat.h>

void mathsat_test(void) {
	using namespace mathsat;
	using std::vector;

	Config conf = Config();
	conf.set("interpolation", true);

	Env env = Env(conf);

	Sort rat = env.rat_sort();
	vector<Sort> empty_params;
	Decl f = env.function("f", rat, rat);
	Decl g = env.function("g", rat, rat);
	Expr x1 = env.rat_const("x1");
	Expr x2 = env.rat_const("x2");
	Expr x3 = env.rat_const("x3");
	Expr y1 = env.rat_const("y1");
	Expr y2 = env.rat_const("y2");
	Expr y3 = env.rat_const("y3");
	Expr b = env.rat_const("b");
	Expr f_x1 = f(x1);
	std::cout << "f(x1): " << f_x1 << std::endl;

	Expr A = (f(x1) + x2 == x3) && (f(y1) + y2 == y3) && (y1 <= x1);
	std::cout << "A: " << A << std::endl;
	Expr B = (g(b) == x2) && (g(b) == y2) && (x1 <= y1) && (x3 < y3);
	std::cout << "B: " << B << std::endl;

	Expr bv1 = env.bv_const("bv1", 1);
	Expr bv0 = env.bv_const("bv0", 1);
	Expr v1 = env.bv_const("v1", 1024);
	Expr v2 = env.bv_const("v2", 1024);
	Expr v3 = env.bv_const("v3", 1024);
	Expr v4 = env.bv_const("v4", 1024);
	Expr v5 = env.bv_const("v5", 1024);
	Expr v6 = env.bv_const("v6", 1024);
	Expr C1 = !((udiv(v1, v2) * udiv(v1, v3) * udiv(v2, v3)) == (v6 * udiv(v4, v5)));
	Expr C = !(ite(C1, bv1, bv0) == bv0);
	std::cout << "C: " << C << std::endl;

	Solver sol = Solver(env);
	InterpolationGroup grA = sol.create_interp_group();
	InterpolationGroup grB = sol.create_interp_group();
	sol.set_interp_group(grA);
	sol.add(A);
	sol.set_interp_group(grB);
	sol.add(B);

	auto res = sol.check();
	std::cout << "res: " << res << std::endl;

	Expr interp = sol.get_interpolant(&grA, 1);
	std::cout << "Interpolant: " << interp << std::endl;
}


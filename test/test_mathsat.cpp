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

#include "SMT/MathSAT/Logic.hpp"
#include "SMT/MathSAT/MathSAT.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "SMT/SMT.hpp"
#include "Util/util.h"

namespace {

using namespace borealis::logging;
using namespace borealis::util;
using namespace borealis::util::streams;

static stream_t infos() {
    return infosFor("test");
}

TEST(MathSAT, constants) {
    using namespace borealis::mathsat;

    Config conf = Config();
    Env env = Env(conf);

    Expr a = env.num_val(1234);
    Expr b = env.num_val(-1234);
    Expr c = env.bv_val(1234, 32);
    Expr d = env.bv_val(-1234, 32);

    auto check_expr = [&](Expr e)->bool {
        Solver solver(e.env());
        solver.add(!e);
        solver.add(e.env().bool_val(true));
        return solver.check() == MSAT_UNSAT;
    };

    EXPECT_TRUE(check_expr(a != b));
    EXPECT_TRUE(check_expr(a == (-b)));
    EXPECT_TRUE(check_expr(c != d));
    EXPECT_TRUE(check_expr(c == (-d)));
}

TEST(MathSAT, generatingFormulas) {
    // Comparing two identical formulas:
    // - first one created using our C++ API
    // - second one created using standard C API

    using namespace borealis::mathsat;

    // C++ API
    Config conf = Config();
    Env env = Env(conf);

    Sort rat = env.rat_sort();
    Decl f = env.function("f", { rat }, rat);
    Expr x1 = env.rat_const("x1");
    Expr x2 = env.rat_const("x2");
    Expr x3 = env.rat_const("x3");
    Expr y1 = env.rat_const("y1");
    Expr y2 = env.rat_const("y2");
    Expr y3 = env.rat_const("y3");

    Expr A = (f(x1) - x2 == -x3) && (f(y1) + y2 == y3) && (y1 <= x1);

    // C API
    msat_term formula;
    formula = msat_from_string(env,
            "(and (= (- (f x1) x2) (- x3))"
            "(= (+ (f y1) y2) y3)"
            "(<= y1 x1))");

    Solver sol(env);
    sol.add(A);
    msat_assert_formula(sol.env(), msat_make_not(env, formula));

    msat_result res = sol.check();
    ASSERT_EQ(MSAT_UNSAT, res);
}

TEST(MathSAT, generatingInterpolant) {
    // Comparing two interpolants for same formulas:
    // - first one generated using our C++ API
    // - second one generated using standard C API

    using namespace borealis::mathsat;

    // C++ API
    Env env(Config::Interpolation());

    Sort rat = env.rat_sort();
    Decl f = env.function("f", { rat }, rat);
    Decl g = env.function("g", { rat }, rat);
    Expr x1 = env.rat_const("x1");
    Expr x2 = env.rat_const("x2");
    Expr x3 = env.rat_const("x3");
    Expr y1 = env.rat_const("y1");
    Expr y2 = env.rat_const("y2");
    Expr y3 = env.rat_const("y3");
    Expr b = env.rat_const("b");

    Expr A = (f(x1) + x2 == x3) && (f(y1) + y2 == y3) && (y1 <= x1);
    Expr B = (g(b) == x2) && (g(b) == y2) && (x1 <= y1) && (x3 < y3);

    ISolver sol(env);
    sol.push();
    auto grA = sol.create_and_set_itp_group();
    sol.add(A);
    sol.create_and_set_itp_group();
    sol.add(B);

    auto res = sol.check();
    ASSERT_EQ(MSAT_UNSAT, res);
    Expr cppInterp = sol.get_interpolant({ grA });
    sol.pop();

    // C API
    msat_term formula;
    msat_push_backtrack_point(sol.env());
    int group_a = msat_create_itp_group(sol.env());
    int group_b = msat_create_itp_group(sol.env());

    formula = msat_from_string(sol.env(),
            "(and (= (+ (f x1) x2) x3)"
            "(= (+ (f y1) y2) y3)"
            "(<= y1 x1))");
    msat_set_itp_group(sol.env(), group_a);
    msat_assert_formula(sol.env(), formula);

    formula = msat_from_string(sol.env(),
            "(and (= x2 (g b))"
            "(= y2 (g b))"
            "(<= x1 y1)"
            "(< x3 y3))");
    msat_set_itp_group(sol.env(), group_b);
    msat_assert_formula(sol.env(), formula);

    res = msat_solve(sol.env());
    ASSERT_EQ(MSAT_UNSAT, res);
    msat_term cInterp = msat_get_interpolant(sol.env(), &group_a, 1);
    msat_pop_backtrack_point(sol.env());

    // Comparing interpolants
    sol.add(cppInterp);
    msat_assert_formula(sol.env(), msat_make_not(sol.env(), cInterp));
    res = sol.check();
    ASSERT_EQ(MSAT_UNSAT, res);
}

TEST(MathSAT, freshConstFunc) {
    // Testing fresh_constant() and fresh_function() methods of Env class

    using namespace borealis::mathsat;

    Config conf = Config();
    Env env = Env(conf);

    Sort rat = env.rat_sort();
    {
        Expr x1 = env.rat_const("x1");
        Expr x2 = env.fresh_constant("x1", rat);
        Expr x3 = env.fresh_constant("x1", rat);
        Expr y = env.fresh_constant("y", rat);

        EXPECT_NE(x1.decl().name(), x2.decl().name());
        EXPECT_NE(x1.decl().name(), x3.decl().name());
        EXPECT_NE(x2.decl().name(), x3.decl().name());
        EXPECT_NE(y.decl().name(), x2.decl().name());
    };

    {
        Decl f1 = env.function("f1", { rat }, rat);
        Decl f2 = env.fresh_function("f1", { rat }, rat);
        Decl f3 = env.fresh_function("f1", { rat }, rat);
        Decl g = env.fresh_function("g", { rat }, rat);

        EXPECT_NE(f1.name(), f2.name());
        EXPECT_NE(f1.name(), f3.name());
        EXPECT_NE(f2.name(), f3.name());
        EXPECT_NE(g.name(), f2.name());
    };
}

TEST(MathSAT, logic) {
    // Testing MathSAT logic
    using namespace borealis::mathsat;
    using namespace borealis::mathsat_::logic;

    Config conf = Config();
    Env env = Env(conf);

    auto check_expr = [&](Bool e)->bool {
        Solver solver(msatimpl::getEnvironment(e));
        solver.add(msatimpl::getAxiom(e));
        solver.add(!msatimpl::getExpr(e));
        return solver.check() == MSAT_UNSAT;
    };

    {
        auto b = Bool::mkConst(env, true);
        auto c = Bool::mkConst(env, false);

        EXPECT_TRUE(check_expr(!(b && c)));
        EXPECT_TRUE(check_expr((b || c)));
        EXPECT_TRUE(check_expr(!(b == c)));
        EXPECT_TRUE(check_expr((b != c)));
    }

    {
        auto d = BitVector<8>::mkConst(env, 0xff);
        auto e = BitVector<8>::mkConst(env, 0xff);

        EXPECT_TRUE(check_expr(d == e));
    }

    {
        auto d = BitVector<16>::mkConst(env, 0x0f);
        auto e = BitVector<8>::mkConst(env, 0x0f);

        EXPECT_TRUE(check_expr(d == e));
    }

    {
        auto d = BitVector<16>::mkVar(env, "pyish", [&env](BitVector<16> v) {
            return (v == BitVector<16>::mkConst(env, 0x0f));
        });
        auto e = BitVector<32>::mkConst(env, 0x0f);

        EXPECT_TRUE(check_expr(d == e));
    }

} // TEST(Solver, logicMathSat)

TEST(MathSAT, memoryArray) {
    {
        using namespace borealis::mathsat_::logic;

        USING_SMT_IMPL(borealis::MathSAT);

        ExprFactory factory;

        auto mkbyte =
                [&](int val) {return Byte::mkConst(factory.unwrap(), val);};

        auto check_expr = [&](Bool e)->bool {
            borealis::mathsat::Solver solver(msatimpl::getEnvironment(e));
            solver.add(msatimpl::getAxiom(e));
            solver.add(!msatimpl::getExpr(e));
            return solver.check() == MSAT_UNSAT;
        };

        factory.getBoolConst(true);
        EXPECT_NO_THROW(factory.getNoMemoryArray());
        EXPECT_NO_FATAL_FAILURE(factory.getNoMemoryArray());

        auto arr = factory.getNoMemoryArray();
        // empty mem is filled with 0xFFs
        for (int i = 0; i < 153; i++) {
            EXPECT_TRUE(check_expr(arr[i] == mkbyte(0xFF)));
        }
    }
}

TEST(MathSAT, mergeMemory) {
    {
        using namespace borealis::mathsat_::logic;

        USING_SMT_IMPL(borealis::MathSAT);

        ExprFactory factory;

        ExecutionContext default_memory(factory);
        ExecutionContext memory_with_a(factory);
        ExecutionContext memory_with_b(factory);

        Pointer ptr = factory.getPtrVar("ptr");
        Integer a = factory.getIntConst(0xdeadbeef);
        Integer b = factory.getIntConst(0xabcdefff);
        Integer z = factory.getIntConst(0xfeedbeef);

        Integer cond = factory.getIntVar("cond");
        Bool cond_a = cond == a;
        Bool cond_b = cond == b;

        memory_with_a.writeExprToMemory(ptr, a);
        memory_with_b.writeExprToMemory(ptr, b);

        ExecutionContext merged = ExecutionContext::mergeMemory("merged",
                default_memory, std::vector<std::pair<Bool, ExecutionContext>> {
                        { cond_a, memory_with_a }, { cond_b, memory_with_b } });
        Integer c = merged.readExprFromMemory<Integer>(ptr);

        auto check_expr_in = [&](Bool e, Bool in)->bool {

            infos() << "Checking:" << endl
            << e << endl
            << "  in:" << endl
            << in << endl;

            borealis::mathsat::Solver s(factory.unwrap());

            s.add(msatimpl::asAxiom(in));

            Bool pred = factory.getBoolVar("$CHECK$");
            s.add(msatimpl::asAxiom(implies(pred, !e)));

            infos() << "$CHECK$:" << endl
            << implies(pred, !e) << endl;

            borealis::mathsat::Expr pred_e = msatimpl::getExpr(pred);
            msat_result r = s.check( {pred_e});

            if (r == MSAT_SAT) {
                infos() << "SAT" << endl;
            } else if (r == MSAT_UNSAT) {
                infos() << "UNSAT" << endl;
            } else {
                infos() << "WTF" << endl;
            }

            return r == MSAT_UNSAT;
        };

        EXPECT_TRUE(check_expr_in(
            c == a,   // expr
            cond == a // in
        ));
        EXPECT_TRUE(check_expr_in(
            c == b,   // expr
            cond == b // in
        ));
        EXPECT_FALSE(check_expr_in(
            c == a,   // expr
            cond == z // in
        ));
    }
}

TEST(MathSAT, Unlogic) {
    using namespace borealis::mathsat;
    using namespace borealis::mathsat_;

    USING_SMT_IMPL(borealis::MathSAT);

    auto check_expr = [](Bool e)->bool {
        borealis::mathsat::Solver solver(logic::msatimpl::getEnvironment(e));
        solver.add(logic::msatimpl::getAxiom(e));
        solver.add(!logic::msatimpl::getExpr(e));
        return solver.check() == MSAT_UNSAT;
    };


    ExprFactory factory;
    Env env = factory.unwrap();

    Sort bv = env.bv_sort(32);
    Decl f = env.function("f", { bv }, bv);
    Expr x1 = env.bv_const("x1", 32);
    Expr x2 = env.bv_const("x2", 32);
    Expr x3 = env.bv_const("x3", 32);
    Expr y1 = env.bv_const("y1", 32);
    Expr y2 = env.bv_const("y2", 32);
    Expr y3 = env.bv_const("y3", 32);

    Expr A = ((x1 + x2 - x3) >= -env.bv_val(32, 32)) || ((y1 * y2 / y3) == (x3 & x1));

    Dynamic dynA(A);

    auto undoed = unlogic::undoThat(dynA);

    ExecutionContext ctx(factory);
    auto redoed = borealis::SMT<borealis::MathSAT>::doit(undoed, factory, &ctx);

    EXPECT_TRUE(check_expr(dynA == Dynamic(redoed)));

}

TEST(MathSAT, diversify) {
    {
        USING_SMT_IMPL(borealis::MathSAT);

        using borealis::mathsat_::logic::msatimpl::asAxiom;
        using borealis::mathsat_::logic::msatimpl::getExpr;

        ExprFactory ef;

        auto a = ef.getIntVar("a");
        auto b = ef.getIntVar("b");
        auto c = ef.getIntVar("c");

        auto zero = ef.getIntConst(0);
        auto one =  ef.getIntConst(1);
        auto two =  ef.getIntConst(2);

        auto query = c != zero;
        auto state = c == ef.if_(a > b)
                            .then_(zero)
                            .else_(one);

        borealis::mathsat::DSolver s(ef.unwrap());
        s.add(asAxiom(query));
        s.add(asAxiom(state));

        auto res = s.check();
        ASSERT_EQ(msat_result::MSAT_SAT, res);

        auto models = s.diversify({getExpr(a), getExpr(b)});
        ASSERT_EQ(4, models.size());
    }
}

} // namespace

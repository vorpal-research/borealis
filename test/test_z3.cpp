/*
 * test_z3.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>
#include <z3/z3++.h>

#include "SMT/Z3/Solver.h"
#include "Util/util.h"

namespace {

using namespace borealis;
using namespace borealis::logging;
using namespace borealis::util;
using namespace borealis::util::streams;

static stream_t infos() {
    return infosFor("test");
}

TEST(Z3ExprFactory, memoryArray) {
    {
        using namespace borealis::z3_::logic;

        USING_SMT_IMPL(Z3);

        ExprFactory factory;
        auto mkbyte = [&](int val){ return Byte::mkConst(factory.unwrap(), val); };

        auto check_expr = [&](Bool e)->bool {
            z3::solver solver(z3impl::getContext(e));
            solver.add(z3impl::getAxiom(e));
            solver.add(!z3impl::getExpr(e));
            return solver.check() == z3::unsat;
        };

        EXPECT_NO_THROW(factory.getNoMemoryArray());
        EXPECT_NO_FATAL_FAILURE(factory.getNoMemoryArray());

        auto arr = factory.getNoMemoryArray();
        // empty mem is filled with 0xFFs
        for (int i = 0; i < 153; i++) {
            EXPECT_TRUE(check_expr(arr[i] == mkbyte(0xFF)));
        }
    }
}

TEST(ExecutionContext, mergeMemory) {
    {
        using namespace borealis::z3_::logic;

        USING_SMT_IMPL(Z3);

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

        ExecutionContext merged = ExecutionContext::mergeMemory(
                "merged",
                default_memory,
                std::vector<std::pair<Bool, ExecutionContext>>{
                    { cond_a, memory_with_a },
                    { cond_b, memory_with_b }
                }
        );
        Integer c = merged.readExprFromMemory<Integer>(ptr);

        auto check_expr_in = [&](Bool e, Bool in)->bool {

            infos() << "Checking:" << endl
                    << e << endl
                    << "  in:" << endl
                    << in << endl;

            z3::solver s(factory.unwrap());

            s.add(z3impl::asAxiom(in));

            Bool pred = factory.getBoolVar("$CHECK$");
            s.add(z3impl::asAxiom(implies(pred, !e)));

            z3::expr pred_e = z3impl::getExpr(pred);
            z3::check_result r = s.check(1, &pred_e);

            if (r == z3::sat) {
                infos() << "SAT:" << endl;
                infos() << s.get_model() << endl;
            } else if (r == z3::unsat) {
                infos() << "UNSAT:" << endl;
                auto core = s.unsat_core();
                for (size_t i = 0U; i < core.size(); ++i) infos() << core[i] << endl;
            } else {
                infos() << "WTF:" << endl;
                infos() << s.reason_unknown() << endl;
            }

            return r == z3::unsat;
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

TEST(Solver, logic) {

    using namespace borealis::z3_::logic;

    USING_SMT_IMPL(Z3);

    z3::context ctx;
    auto check_expr = [&](Bool e)->bool {
        z3::solver solver(z3impl::getContext(e));
        solver.add(z3impl::getAxiom(e));
        solver.add(!z3impl::getExpr(e));
        return solver.check() == z3::unsat;
    };

    {
        auto b = Bool::mkConst(ctx, true);
        auto c = Bool::mkConst(ctx, false);

        EXPECT_TRUE(check_expr( ! (b && c) ));
        EXPECT_TRUE(check_expr(   (b || c) ));
        EXPECT_TRUE(check_expr( ! (b == c) ));
        EXPECT_TRUE(check_expr(   (b != c) ));
    }

    {
        auto d = BitVector<8>::mkConst(ctx, 0xff);
        auto e = BitVector<8>::mkConst(ctx, 0xff);

        EXPECT_TRUE(check_expr(d == e));
    }

    {
        auto d = BitVector<16>::mkConst(ctx, 0x0f);
        auto e = BitVector<8>::mkConst(ctx, 0x0f);

        EXPECT_TRUE(check_expr(d == e));
    }

    {
        auto d = BitVector<16>::mkVar(ctx, "pyish", [&ctx](BitVector<16> v){
            return (v == BitVector<16>::mkConst(ctx, 0x0f));
        });
        auto e = BitVector<32>::mkConst(ctx, 0x0f);

        EXPECT_TRUE(check_expr(d == e));
    }

    {
        auto id = [](BitVector<32> bv){
            return bv;
        };
        auto f = Function<BitVector<32>(BitVector<32>)>(ctx, "f", id);

        auto e = BitVector<32>::mkConst(ctx, 0x0f);

        EXPECT_TRUE(check_expr(f(e) == e));
    }

    {
        auto is0x0f = [&ctx](BitVector<32> bv){
            return (bv == BitVector<32>::mkConst(ctx, 0x0f));
        };
        auto f = Function<Bool(BitVector<32>)>(ctx, "f", is0x0f);

        auto v0x0f = BitVector<32>::mkVar(ctx, "v0x0f", [&ctx](BitVector<32> v){
            return v == BitVector<32>::mkConst(ctx, 0x0f);
        });

        EXPECT_TRUE(check_expr(f(v0x0f)));
    }

} // TEST(Z3ExprFactory, logic)

} // namespace

/*
 * test_z3.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>
#include <z3/z3++.h>

#include "SMT/Z3/Divers.h"
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

TEST(Z3, diversify) {
    USING_SMT_IMPL(borealis::Z3);

    using borealis::z3_::logic::z3impl::asAxiom;
    using borealis::z3_::logic::z3impl::getExpr;

    ExprFactory ef;

    auto&& a = ef.getIntVar("a");
    auto&& b = ef.getIntVar("b");
    auto&& c = ef.getIntVar("c");

    auto&& zero = ef.getIntConst(0);
    auto&& one =  ef.getIntConst(1);
    // auto&& two =  ef.getIntConst(2);

    auto&& query = c != zero;
    auto&& state = c == ef.if_(a > b)
                          .then_(zero)
                          .else_(one);

    z3::solver s{ ef.unwrap() };
    s.add(asAxiom(query));
    s.add(asAxiom(state));

    auto&& res = s.check();
    ASSERT_EQ(z3::sat, res);

    auto&& models = z3::diversify(s, {getExpr(a), getExpr(b)}, {getExpr(a), getExpr(b)});
    ASSERT_EQ(32, models.size());
}

TEST(Z3ExprFactory, memoryArray) {
    {
        using namespace borealis::z3_::logic;

        USING_SMT_IMPL(Z3);

        ExprFactory factory;
        auto&& mkbyte = [&](auto&& val) { return Byte::mkConst(factory.unwrap(), val); };

        auto&& check_expr = [&](auto&& e) {
            z3::solver solver{ z3impl::getContext(e) };
            solver.add(z3impl::getAxiom(e));
            solver.add(not z3impl::getExpr(e));
            return z3::unsat == solver.check();
        };

        EXPECT_NO_THROW(factory.getNoMemoryArray("mem"));
        EXPECT_NO_FATAL_FAILURE(factory.getNoMemoryArray("mem"));

        auto&& arr = factory.getNoMemoryArray("mem");
        // empty mem is filled with 0xFFs
        for (auto&& i = 0; i < 129; ++i) {
            EXPECT_TRUE(check_expr(arr[i] == mkbyte(0xFF)));
        }
    }
}

TEST(ExecutionContext, mergeMemory) {
    {
        using namespace borealis::z3_::logic;

        USING_SMT_IMPL(Z3);

        ExprFactory factory;

        ExecutionContext default_memory(factory, (1 << 16) + 1, (2 << 16) + 1);
        ExecutionContext memory_with_a(factory,  (1 << 16) + 1, (2 << 16) + 1);
        ExecutionContext memory_with_b(factory,  (1 << 16) + 1, (2 << 16) + 1);

        auto&& ptr = factory.getPtrVar("ptr");
        auto&& a = factory.getIntConst(0xdeadbeef);
        auto&& b = factory.getIntConst(0xabcdefff);
        auto&& z = factory.getIntConst(0xfeedbeef);

        auto&& cond = factory.getIntVar("cond");
        auto&& cond_a = cond == a;
        auto&& cond_b = cond == b;

        memory_with_a.writeExprToMemory(ptr, a);
        memory_with_b.writeExprToMemory(ptr, b);

        auto&& merged = ExecutionContext::mergeMemory(
                "merged",
                default_memory,
                std::vector<std::pair<Bool, ExecutionContext>>{
                    { cond_a, memory_with_a },
                    { cond_b, memory_with_b }
                }
        );
        auto&& c = merged.readExprFromMemory<Byte>(ptr);

        auto&& check_expr_in = [&](auto&& e, auto&& in) {

            infos() << "Checking:" << endl
                    << e << endl
                    << "  in:" << endl
                    << in << endl;

            z3::solver s{ factory.unwrap() };

            s.add(z3impl::asAxiom(in));

            auto&& pred = factory.getBoolVar("$CHECK$");
            s.add(z3impl::asAxiom(implies(pred, not e)));

            infos() << "$CHECK$:" << endl
            		<< implies(pred, not e) << endl;

            auto&& pred_e = z3impl::getExpr(pred);
            auto&& r = s.check(1, &pred_e);

            if (r == z3::sat) {
                infos() << "SAT:" << endl;
                infos() << s.get_model() << endl;
            } else if (r == z3::unsat) {
                infos() << "UNSAT:" << endl;
                auto&& core = s.unsat_core();
                for (auto&& i = 0U; i < core.size(); ++i) infos() << core[i] << endl;
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
    auto&& check_expr = [&](auto&& e) {
        z3::solver solver{ z3impl::getContext(e) };
        solver.add(z3impl::getAxiom(e));
        solver.add(!z3impl::getExpr(e));
        auto&& res = solver.check();
        return res == z3::unsat;
    };

    {
        auto&& b = Bool::mkConst(ctx, true);
        auto&& c = Bool::mkConst(ctx, false);

        EXPECT_TRUE(check_expr( ! (b && c) ));
        EXPECT_TRUE(check_expr(   (b || c) ));
        EXPECT_TRUE(check_expr( ! (b == c) ));
        EXPECT_TRUE(check_expr(   (b != c) ));
    }

    {
        auto&& d = BitVector<8>::mkConst(ctx, 0xff);
        auto&& e = BitVector<8>::mkConst(ctx, 0xff);

        EXPECT_TRUE(check_expr(d == e));
    }

    {
        auto&& d = BitVector<16>::mkConst(ctx, 0x0f);
        auto&& e = BitVector<8>::mkConst(ctx, 0x0f);

        EXPECT_TRUE(check_expr(d == e));
    }

    {
        auto&& d = BitVector<16>::mkVar(ctx, "pyish", [&](auto&& v) {
            return (v == BitVector<16>::mkConst(ctx, 0x0f));
        });
        auto&& e = BitVector<32>::mkConst(ctx, 0x0f);
        auto&& res = d == e;
        EXPECT_TRUE(check_expr(res));
    }

    {
        auto&& id = [](auto&& bv) { return bv; };
        auto&& f = Function<BitVector<32>(BitVector<32>)>(ctx, "f", id);

        auto&& e = BitVector<32>::mkConst(ctx, 0x0f);

        EXPECT_TRUE(check_expr(f(e) == e));
    }

    {
        auto&& is0x0f = [&](auto&& bv){
            return (bv == BitVector<32>::mkConst(ctx, 0x0f));
        };
        auto&& f = Function<Bool(BitVector<32>)>(ctx, "f", is0x0f);

        auto&& v0x0f = BitVector<32>::mkVar(ctx, "v0x0f", [&](auto&& v) {
            return v == BitVector<32>::mkConst(ctx, 0x0f);
        });

        EXPECT_TRUE(check_expr(f(v0x0f)));
    }

} // TEST(Z3ExprFactory, logic)

} // namespace

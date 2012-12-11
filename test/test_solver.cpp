/*
 * test_term.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>
#include <z3/z3++.h>

#include "Util/util.h"
#include "Solver/Z3ExprFactory.h"
#include "Solver/Logic.hpp"

namespace {

using namespace borealis;
using namespace borealis::logging;
using namespace borealis::util;
using namespace borealis::util::streams;

static stream_t infos() {
    return infosFor("test");
}

TEST(Z3ExprFactory, getMemoryArray) {

    {
        z3::context con;
        borealis::Z3ExprFactory factory(con);
        auto mkbyte = [&](int val){ return con.bv_val(val, 8); };
        auto mkptr = [&](int val){ return con.bv_val(val, factory.getPtrSort().bv_size()); };
        auto check_expr = [&](z3::expr axioms, z3::expr e)->bool {
            z3::solver solver(e.ctx());
            solver.add(axioms);
            solver.add(!e);
            return solver.check() == z3::unsat;
        };

        EXPECT_NO_THROW(factory.getNoMemoryArray());
        EXPECT_NO_FATAL_FAILURE(factory.getNoMemoryArray());

        auto arr = factory.getNoMemoryArray();

        // empty mem is filled with 0xff's
        for (int i = 0; i < 153; i+=7) {
            EXPECT_TRUE(check_expr(factory.getNoMemoryArrayAxiom(arr), arr(i) == mkbyte(0xff)));
        }

    }

}

TEST(Z3ExprFactory, byteFucking) {

    {
        z3::context con;
        borealis::Z3ExprFactory factory(con);
        auto mkbyte = [&](int val){ return con.bv_val(val, 8); };
        auto mkptr = [&](int val){ return con.bv_val(val, factory.getPtrSort().bv_size()); };
        auto check_expr = [&](z3::expr axioms, z3::expr e)->bool {
            z3::solver solver(e.ctx());
            solver.add(axioms);
            solver.add(!e);
            return solver.check() == z3::unsat;
        };

        {
            auto intc  = con.bv_val(0xaffa, 16);
            auto bytes = factory.splitBytes(intc);

            EXPECT_TRUE(check_expr(factory.getBoolConst(true), bytes[0] == mkbyte(0xfa)));
            EXPECT_TRUE(check_expr(factory.getBoolConst(true), bytes[1] == mkbyte(0xaf)));

            auto check = factory.concatBytes(bytes);

            EXPECT_TRUE(check_expr(factory.getBoolConst(true), intc == check));
        }

        {
            auto intc  = con.bv_val(0xcafebabe, 32);
            auto bytes = factory.splitBytes(intc);

            EXPECT_TRUE(check_expr(factory.getBoolConst(true), bytes[0] == mkbyte(0xbe)));
            EXPECT_TRUE(check_expr(factory.getBoolConst(true), bytes[1] == mkbyte(0xba)));
            EXPECT_TRUE(check_expr(factory.getBoolConst(true), bytes[2] == mkbyte(0xfe)));
            EXPECT_TRUE(check_expr(factory.getBoolConst(true), bytes[3] == mkbyte(0xca)));

            auto check = factory.concatBytes(bytes);

            EXPECT_TRUE(check_expr(factory.getBoolConst(true), intc == check));
        }

        {
            // concatenating an empty vector results in 1-bit zero bv
            auto single = factory.concatBytes(std::vector<z3::expr>());
            EXPECT_TRUE(check_expr(factory.getBoolConst(true), single == con.bv_val(0,1)));
        }

        {
            auto mem = factory.getNoMemoryArray();
            auto filled = factory.byteArrayInsert(mem, mkptr(26), con.bv_val(0xcafebabe, 32));

            auto extr = factory.byteArrayExtract(filled.first, mkptr(26), 4);
            EXPECT_TRUE(check_expr(factory.getNoMemoryArrayAxiom(mem) && filled.second,
                    extr == con.bv_val(0xcafebabe, 32)));
        }
    }

}

TEST(Z3ExprFactory, logic) {
    using borealis::logic::Bool;
    using borealis::logic::BitVector;
    using borealis::logic::Function;

    z3::context ctx;
    auto check_expr = [&](Bool e)->bool {
        z3::solver solver(e.get().ctx());
        solver.add(e.axiom());
        solver.add(!e.get());
        return solver.check() == z3::unsat;
    };

    auto b = Bool::mkConst(ctx, true);
    auto c = Bool::mkConst(ctx, false);

    EXPECT_TRUE(check_expr(!(b&&c)));
    EXPECT_TRUE(check_expr((b||c)));
    EXPECT_TRUE(check_expr(!(b==c)));
    EXPECT_TRUE(check_expr((b!=c)));

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
        auto id = [](BitVector<32> bv){ return bv; };
        auto f = Function<BitVector<32>(BitVector<32>)>(ctx, "f", id);

        auto e = BitVector<32>::mkConst(ctx, 0x0f);

        EXPECT_TRUE(check_expr(f(e) == e));
    }

    {
        auto id = [&ctx](BitVector<32> bv){ return (bv == BitVector<16>::mkConst(ctx, 0x0f)); };
        auto f = Function<Bool(BitVector<32>)>(ctx, "f", id);
        auto v0x0f = BitVector<32>::mkVar(ctx, "v0x0f", [&ctx](BitVector<32> v){
            return v == BitVector<32>::mkConst(ctx, 0x0f);
        });

        EXPECT_TRUE(check_expr(f(v0x0f)));
    }

}

} // namespace borealis

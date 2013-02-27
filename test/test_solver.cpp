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
//    {
//        using borealis::logic::Bool;
//        using namespace borealis::logic::z3impl;
//        using borealis::Z3ExprFactory;
//
//        typedef Z3ExprFactory::Byte Byte;
//
//        z3::context con;
//        Z3ExprFactory factory(con);
//        auto mkbyte = [&](int val){ return Byte::mkConst(con, val); };
//
//        auto check_expr = [&](Bool e)->bool {
//            z3::solver solver(getContext(e));
//            solver.add(getAxiom(e));
//            solver.add(!getExpr(e));
//            return solver.check() == z3::unsat;
//        };
//
//        EXPECT_NO_THROW(factory.getNoMemoryArray());
//        EXPECT_NO_FATAL_FAILURE(factory.getNoMemoryArray());
//
//        auto arr = factory.getNoMemoryArray();
//
//        // empty mem is filled with 0xff's
//        for (int i = 0; i < 153; i+=7) {
//            EXPECT_TRUE(check_expr(arr[i] == mkbyte(0xff)));
//        }
//    }
}

TEST(Z3ExprFactory, logic) {
//    using borealis::logic::Bool;
//
//    using namespace borealis::logic::z3impl;
//
//    using borealis::logic::BitVector;
//    using borealis::logic::Function;
//
//    z3::context ctx;
//    auto check_expr = [&](Bool e)->bool {
//        z3::solver solver(getContext(e));
//        solver.add(getAxiom(e));
//        solver.add(!getExpr(e));
//        return solver.check() == z3::unsat;
//    };
//
//    auto b = Bool::mkConst(ctx, true);
//    auto c = Bool::mkConst(ctx, false);
//
//    EXPECT_TRUE(check_expr(!(b&&c)));
//    EXPECT_TRUE(check_expr((b||c)));
//    EXPECT_TRUE(check_expr(!(b==c)));
//    EXPECT_TRUE(check_expr((b!=c)));
//
//    {
//        auto d = BitVector<8>::mkConst(ctx, 0xff);
//        auto e = BitVector<8>::mkConst(ctx, 0xff);
//
//        EXPECT_TRUE(check_expr(d == e));
//    }
//
//    {
//        auto d = BitVector<16>::mkConst(ctx, 0x0f);
//        auto e = BitVector<8>::mkConst(ctx, 0x0f);
//
//        EXPECT_TRUE(check_expr(d == e));
//    }
//
//    {
//        auto d = BitVector<16>::mkVar(ctx, "pyish", [&ctx](BitVector<16> v){
//            return (v == BitVector<16>::mkConst(ctx, 0x0f));
//        });
//        auto e = BitVector<32>::mkConst(ctx, 0x0f);
//
//
//        EXPECT_TRUE(check_expr(d == e));
//    }
//
//    {
//        auto id = [](BitVector<32> bv){ return bv; };
//        auto f = Function<BitVector<32>(BitVector<32>)>(ctx, "f", id);
//
//        auto e = BitVector<32>::mkConst(ctx, 0x0f);
//
//        EXPECT_TRUE(check_expr(f(e) == e));
//    }
//
//    {
//        auto id = [&ctx](BitVector<32> bv){ return (bv == BitVector<32>::mkConst(ctx, 0x0f)); };
//        auto f = Function<Bool(BitVector<32>)>(ctx, "f", id);
//        auto v0x0f = BitVector<32>::mkVar(ctx, "v0x0f", [&ctx](BitVector<32> v){
//            return v == BitVector<32>::mkConst(ctx, 0x0f);
//        });
//
//        EXPECT_TRUE(check_expr(f(v0x0f)));
//    }
}

} // namespace borealis

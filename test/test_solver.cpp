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

} // namespace borealis

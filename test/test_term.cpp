/*
 * test_term.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include "Predicate/ArgumentTerm.h"
#include "Predicate/ConstTerm.h"
#include "Predicate/ReturnValueTerm.h"
#include "Predicate/Term.h"
#include "Predicate/ValueTerm.h"
#include "Util/slottracker.h"
#include "Util/util.h"

namespace {

using namespace borealis::util;
using namespace borealis::util::streams;

TEST(Term, classof) {

    {
        using namespace borealis;

        using llvm::LLVMContext;
        using llvm::GlobalValue;
        using llvm::Module;
        using llvm::Value;
        using llvm::dyn_cast;
        using llvm::isa;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);
        Value* v = GlobalValue::getNullValue(llvm::Type::getInt1Ty(ctx));
        SlotTracker st(&m);

        Term* t1 = new ArgumentTerm(v, &st);

        EXPECT_TRUE(isa<ArgumentTerm>(t1));
        EXPECT_TRUE(isa<Term>(t1));
        EXPECT_FALSE(isa<ConstTerm>(t1));
        EXPECT_FALSE(isa<ReturnValueTerm>(t1));
        EXPECT_FALSE(isa<ValueTerm>(t1));

        ConstTerm* t2 = dyn_cast<ConstTerm>(t1);
        EXPECT_EQ(nullptr, t2);
    }

}

} // namespace borealis

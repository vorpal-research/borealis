/*
 * test_term.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <gtest/gtest.h>

#include "Factory/Nest.h"
#include "Term/Term.def"
#include "Util/slottracker.h"
#include "Util/util.h"

namespace {

using namespace borealis;
using namespace borealis::util;
using namespace borealis::util::streams;

TEST(Term, classof) {

    {
        using namespace llvm;
        using llvm::Type;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);

        Function* f = Function::Create(
                FunctionType::get(
                        Type::getVoidTy(ctx),
                        Type::getInt1Ty(ctx),
                        false),
                GlobalValue::LinkageTypes::ExternalLinkage);
        Argument* a = &head(f->getArgumentList());
        a->setName("mock-arg");

        SlotTracker st(&m);
        auto TF = FactoryNest(&st).Term;

        auto t1 = TF->getArgumentTerm(a);

        EXPECT_TRUE(isa<ArgumentTerm>(t1));
        EXPECT_TRUE(isa<Term>(t1));
        EXPECT_FALSE(isa<ConstTerm>(t1));
        EXPECT_FALSE(isa<ReturnValueTerm>(t1));
        EXPECT_FALSE(isa<ValueTerm>(t1));

        auto* t2 = dyn_cast<ConstTerm>(t1);
        EXPECT_EQ(nullptr, t2);
    }

}

} // namespace

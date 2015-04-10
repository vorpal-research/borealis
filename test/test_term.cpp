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

TEST(Term, classof) {

    {
        llvm::LLVMContext& ctx = llvm::getGlobalContext();
        llvm::Module m{ "mock-module", ctx };

        auto* f = llvm::Function::Create(
            llvm::FunctionType::get(
                llvm::Type::getVoidTy(ctx),
                llvm::Type::getInt1Ty(ctx),
                false
            ),
            llvm::GlobalValue::LinkageTypes::ExternalLinkage
        );
        auto&& arg = *f->arg_begin();
        arg.setName("mock-arg");

        SlotTracker st{ &m };
        auto&& TF = FactoryNest(&st).Term;

        auto&& t1 = TF->getArgumentTerm(&arg);

        EXPECT_TRUE(llvm::isa<ArgumentTerm>(t1));
        EXPECT_TRUE(llvm::isa<Term>(t1));
        EXPECT_FALSE(llvm::isa<ConstTerm>(t1));
        EXPECT_FALSE(llvm::isa<ReturnValueTerm>(t1));
        EXPECT_FALSE(llvm::isa<ValueTerm>(t1));

        auto* t2 = llvm::dyn_cast<ConstTerm>(t1);
        EXPECT_EQ(nullptr, t2);
    };

}

TEST(Term, equals) {

    {
        llvm::LLVMContext& ctx = llvm::getGlobalContext();
        llvm::Module m{ "mock-module", ctx };

        auto* f = llvm::Function::Create(
            llvm::FunctionType::get(
                llvm::Type::getVoidTy(ctx),
                llvm::Type::getInt1Ty(ctx),
                false
            ),
            llvm::GlobalValue::LinkageTypes::ExternalLinkage
        );
        auto&& arg = *f->arg_begin();
        arg.setName("mock-arg");

        SlotTracker st{ &m };
        auto&& TF = FactoryNest(&st).Term;

        auto&& t1 = TF->getReturnValueTerm(f);
        auto&& t2 = TF->getReturnValueTerm(f);
        EXPECT_EQ(*t1, *t2);
    };

}

} // namespace

/*
 * test_term.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include <llvm/LLVMContext.h>
#include <llvm/Argument.h>
#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Type.h>
#include <llvm/Value.h>

#include <gtest/gtest.h>

#include "Predicate/PredicateFactory.h"
#include "State/CallSiteInitializer.h"
#include "Term/ArgumentTerm.h"
#include "Term/ConstTerm.h"
#include "Term/ReturnValueTerm.h"
#include "Term/Term.h"
#include "Term/TermFactory.h"
#include "Term/ValueTerm.h"
#include "Util/slottracker.h"
#include "Util/util.h"

namespace {

using namespace borealis::util;
using namespace borealis::util::streams;

TEST(Transformer, CallSiteInitializer) {

    {
        using namespace borealis;
        using namespace llvm;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);

        Type* retType = Type::getVoidTy(ctx);
        Type* argType = Type::getInt1Ty(ctx);
        Function* F = Function::Create(
                FunctionType::get(
                        retType,
                        ArrayRef<Type*>(argType),
                        false),
                GlobalValue::LinkageTypes::ExternalLinkage);
        Argument* arg = &*F->getArgumentList().begin();
        arg->setName("mock-arg");
        Value* val_0 = GlobalValue::getIntegerValue(
                argType,
                APInt(1,0));
        val_0->setName("mock-val-0");
        Value* val_1 = GlobalValue::getIntegerValue(
                argType,
                APInt(1,1));
        val_1->setName("mock-val-1");

        CallInst* CI = CallInst::Create(
                F,
                ArrayRef<Value*>(val_0));

        SlotTracker st(&m);

        auto PF = PredicateFactory::get(&st);
        auto TF = TermFactory::get(&st);

        CallSiteInitializer csi(*CI, TF.get());

        auto pred = PF->getEqualityPredicate(
                TF->getArgumentTerm(arg),
                TF->getValueTerm(val_1));
        EXPECT_EQ("mock-arg=1", pred->toString());

        auto pred2 = csi.transform(pred);
        EXPECT_EQ("0=1", pred2->toString());
    }

}

} // namespace borealis

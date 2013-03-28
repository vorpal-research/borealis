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
#include "State/ConstantPropagator.h"
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

        typedef llvm::Type Type;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);

        Type* retType = Type::getVoidTy(ctx);
        Type* argType = Type::getInt1Ty(ctx);
        Function* F = Function::Create(
                FunctionType::get(
                        retType,
                        ArrayRef<llvm::Type*>(argType),
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
        EXPECT_EQ("mock-arg=true", pred->toString());

        auto pred2 = csi.transform(pred);
        EXPECT_EQ("false=true", pred2->toString());
    }

}

TEST(Transformer, ConstantPropagatorUnary) {
    {
        using namespace llvm;
        using namespace borealis;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);
        SlotTracker st(&m);
        auto PF = PredicateFactory::get(&st);
        auto TF = TermFactory::get(&st);

        ConstantFP *d = ConstantFP::get(ctx, APFloat(5.4));
        auto testTerm = TF->getUnaryTerm(UnaryArithType::NEG, TF->getConstTerm(d));

        ConstantPropagator cp(TF.get());
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_FALSE(result == 0);
        ASSERT_DOUBLE_EQ(-5.4, result->getValue());
    }
}

TEST(Transformer, ConstantPropagatorBinary1) {
    {
        using namespace llvm;
        using namespace borealis;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);
        SlotTracker st(&m);
        auto PF = PredicateFactory::get(&st);
        auto TF = TermFactory::get(&st);

        ConstantPropagator cp(TF.get());

        auto term1 = TF->getUnaryTerm(UnaryArithType::NEG,
                                      TF->getConstTerm(ConstantFP::get(ctx, APFloat(5.4))));
        auto term2 = TF->getConstTerm(ConstantFP::get(ctx, APFloat(8.4)));
        auto testTerm = TF->getBinaryTerm(ArithType::ADD, term1, term2);
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_FALSE(result == 0);
        ASSERT_DOUBLE_EQ(3.0, result->getValue());
    }
}

TEST(Transformer, ConstantPropagatorBinary2) {
    {
        using namespace llvm;
        using namespace borealis;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);
        SlotTracker st(&m);
        auto PF = PredicateFactory::get(&st);
        auto TF = TermFactory::get(&st);

        ConstantFP *t1 = ConstantFP::get(ctx, APFloat(5.4));
        auto term1 = TF->getUnaryTerm(UnaryArithType::NEG, TF->getConstTerm(t1));
        ConstantInt *t2 = ConstantInt::get(ctx, APInt(32, 20));
        auto term2 = TF->getConstTerm(t2);
        auto term3 = TF->getBinaryTerm(ArithType::DIV, term2, term1);
        auto term4 = TF->getOpaqueConstantTerm(5LL);
        auto testTerm = TF->getBinaryTerm(ArithType::SUB, term4, term3);

        ConstantPropagator cp(TF.get());
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_FALSE(result == 0);
        ASSERT_NEAR(8.7, result->getValue(), 0.1);
    }
}

TEST(Transformer, ConstantPropagator) {
    {
        using namespace llvm;
        using namespace borealis;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);
        SlotTracker st(&m);
        auto PF = PredicateFactory::get(&st);
        auto TF = TermFactory::get(&st);

        ConstantFP *t1 = ConstantFP::get(ctx, APFloat(5.4));
        auto term1 = TF->getUnaryTerm(UnaryArithType::NOT, TF->getConstTerm(t1));
        ConstantInt *t2 = ConstantInt::get(ctx, APInt(32, 20));
        auto term2 = TF->getConstTerm(t2);
        auto term3 = TF->getBinaryTerm(ArithType::LAND, term2, term1);
        auto term4 = TF->getOpaqueConstantTerm(0.3);
        auto testTerm = TF->getCmpTerm(ConditionType::GTE, term4, term3);

        ConstantPropagator cp(TF.get());
        auto result = dyn_cast<OpaqueBoolConstantTerm>(cp.transform(testTerm));

        ASSERT_FALSE(result == 0);
        ASSERT_TRUE(result->getValue());
    }
}

} // namespace borealis

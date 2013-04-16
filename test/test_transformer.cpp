/*
 * test_transformer.cpp
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
#include "Term/Term.def"
#include "Term/TermFactory.h"
#include "Util/slottracker.h"
#include "Util/util.h"

namespace {

using namespace borealis;
using namespace borealis::util;
using namespace borealis::util::streams;

class TransformerTest : public ::testing::Test {
protected:

    typedef std::unique_ptr<llvm::Module> ModulePtr;
    typedef std::unique_ptr<SlotTracker> SlotTrackerPtr;

    virtual void SetUp() {
        ctx = &llvm::getGlobalContext();
        M = ModulePtr(new llvm::Module("mock-module", *ctx));
        ST = SlotTrackerPtr(new SlotTracker(M.get()));
        PF = PredicateFactory::get(ST.get());
        TF = TermFactory::get(ST.get());
    }

    llvm::LLVMContext* ctx;
    ModulePtr M;
    SlotTrackerPtr ST;
    PredicateFactory::Ptr PF;
    TermFactory::Ptr TF;

};

TEST_F(TransformerTest, CallSiteInitializer) {
    {
        using namespace llvm;
        using llvm::Type;

        Type* retType = Type::getVoidTy(*ctx);
        Type* argType = Type::getInt1Ty(*ctx);
        Function* F = Function::Create(
                FunctionType::get(
                        retType,
                        argType,
                        false),
                GlobalValue::LinkageTypes::ExternalLinkage);

        Argument* arg = &head(F->getArgumentList());
        arg->setName("mock-arg");

        Value* val_0 = GlobalValue::getIntegerValue(argType, APInt(1,0));
        val_0->setName("mock-val-0");

        Value* val_1 = GlobalValue::getIntegerValue(argType, APInt(1,1));
        val_1->setName("mock-val-1");

        CallInst* CI = CallInst::Create(F, val_0);
        CallSiteInitializer csi(*CI, TF.get());

        auto pred = PF->getEqualityPredicate(
                TF->getArgumentTerm(arg),
                TF->getValueTerm(val_1)
        );
        EXPECT_EQ("mock-arg=true", pred->toString());

        auto pred2 = csi.transform(pred);
        EXPECT_EQ("false=true", pred2->toString());
    }
}

TEST_F(TransformerTest, ConstantPropagatorUnary) {
    {
        using namespace llvm;

        // -5.4
        auto testTerm = TF->getUnaryTerm(
                UnaryArithType::NEG,
                TF->getConstTerm(
                        ConstantFP::get(*ctx, APFloat(5.4))
                )
        );

        ConstantPropagator cp(TF.get());
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_NE(0, result);
        EXPECT_DOUBLE_EQ(-5.4, result->getValue());
    }
}

TEST_F(TransformerTest, ConstantPropagatorBinary) {

    {
        using namespace llvm;

        // -5.4 + 8.4
        auto testTerm = TF->getBinaryTerm(
            ArithType::ADD,
            TF->getUnaryTerm(
                UnaryArithType::NEG,
                TF->getConstTerm(
                        ConstantFP::get(*ctx, APFloat(5.4))
                )
            ),
            TF->getConstTerm(
                ConstantFP::get(*ctx, APFloat(8.4))
            )
        );

        ConstantPropagator cp(TF.get());
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_NE(0, result);
        EXPECT_DOUBLE_EQ(3.0, result->getValue());
    }

    {
        using namespace llvm;

        // 5 - (20 / -5.4)
        auto testTerm = TF->getBinaryTerm(
            ArithType::SUB,
            TF->getOpaqueConstantTerm(5LL),
            TF->getBinaryTerm(
                ArithType::DIV,
                TF->getConstTerm(
                    ConstantInt::get(*ctx, APInt(32, 20))
                ),
                TF->getUnaryTerm(
                    UnaryArithType::NEG,
                    TF->getConstTerm(
                        ConstantFP::get(*ctx, APFloat(5.4))
                    )
                )
            )
        );

        ConstantPropagator cp(TF.get());
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_NE(0, result);
        EXPECT_NEAR(8.7, result->getValue(), 0.1);
    }

}

TEST_F(TransformerTest, ConstantPropagator) {
    {
        using namespace llvm;

        // 0.3 >= 20 && !(-5.4)
        auto testTerm = TF->getCmpTerm(
            ConditionType::GTE,
            TF->getOpaqueConstantTerm(0.3),
            TF->getBinaryTerm(
                ArithType::LAND,
                TF->getConstTerm(
                    ConstantInt::get(*ctx, APInt(32, 20))
                ),
                TF->getUnaryTerm(
                    UnaryArithType::NOT,
                    TF->getConstTerm(
                        ConstantFP::get(*ctx, APFloat(5.4))
                    )
                )
            )
        );

        ConstantPropagator cp(TF.get());
        auto result = dyn_cast<OpaqueBoolConstantTerm>(cp.transform(testTerm));

        ASSERT_NE(0, result);
        EXPECT_TRUE(result->getValue());
    }
}

} // namespace

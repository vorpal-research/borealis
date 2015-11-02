/*
 * test_transformer.cpp
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
#include "State/Transformer/CallSiteInitializer.h"
#include "State/Transformer/ConstantPropagator.h"
#include "Term/Term.def"
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
        FN = FactoryNest(M->getDataLayout(), ST.get());
    }

    llvm::LLVMContext* ctx;
    ModulePtr M;
    SlotTrackerPtr ST;
    FactoryNest FN;

};

TEST_F(TransformerTest, CallSiteInitializer) {
    {
        using namespace llvm;
        using llvm::Type;

        Function* Main = Function::Create(
            FunctionType::get(
                Type::getVoidTy(*ctx),
                false
            ),
            GlobalValue::LinkageTypes::InternalLinkage,
            "main"
        );
        BasicBlock* BB = BasicBlock::Create(*ctx, "bb.test", Main);

        Type* retType = Type::getVoidTy(*ctx);
        Type* argType = Type::getInt1Ty(*ctx);
        Function* F = Function::Create(
            FunctionType::get(
                retType,
                argType,
                false),
            GlobalValue::LinkageTypes::ExternalLinkage,
            "test"
        );

        Argument* arg = &head(F->getArgumentList());
        arg->setName("mock-arg");

        Value* val_0 = GlobalValue::getIntegerValue(argType, APInt(1,0));
        val_0->setName("mock-val-0");

        Value* val_1 = GlobalValue::getIntegerValue(argType, APInt(1,1));
        val_1->setName("mock-val-1");

        CallInst* CI = CallInst::Create(F, val_0, "", BB);
        CallSiteInitializer csi(CI, FN);

        auto pred = FN.Predicate->getEqualityPredicate(
            FN.Term->getArgumentTerm(arg),
            FN.Term->getValueTerm(val_1)
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
        auto testTerm = FN.Term->getUnaryTerm(
            UnaryArithType::NEG,
            FN.Term->getConstTerm(
                ConstantFP::get(*ctx, APFloat(5.4))
            )
        );

        ConstantPropagator cp(FN);
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_NE(nullptr, result);
        EXPECT_DOUBLE_EQ(-5.4, result->getValue());
    }
}

TEST_F(TransformerTest, ConstantPropagatorBinary) {

    {
        using namespace llvm;

        // -5.4 + 8.4
        auto testTerm = FN.Term->getBinaryTerm(
            ArithType::ADD,
            FN.Term->getUnaryTerm(
                UnaryArithType::NEG,
                FN.Term->getConstTerm(
                    ConstantFP::get(*ctx, APFloat(5.4))
                )
            ),
            FN.Term->getConstTerm(
                ConstantFP::get(*ctx, APFloat(8.4))
            )
        );

        ConstantPropagator cp(FN);
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_NE(nullptr, result);
        EXPECT_DOUBLE_EQ(3.0, result->getValue());
    }

    {
        using namespace llvm;

        // 5 - (20 / -5.4)
        auto testTerm = FN.Term->getBinaryTerm(
            ArithType::SUB,
            FN.Term->getOpaqueConstantTerm(5, 0x0),
            FN.Term->getBinaryTerm(
                ArithType::DIV,
                FN.Term->getConstTerm(
                    ConstantInt::get(*ctx, APInt(32, 20))
                ),
                FN.Term->getUnaryTerm(
                    UnaryArithType::NEG,
                    FN.Term->getConstTerm(
                        ConstantFP::get(*ctx, APFloat(5.4))
                    )
                )
            )
        );

        ConstantPropagator cp(FN);
        auto result = dyn_cast<OpaqueFloatingConstantTerm>(cp.transform(testTerm));

        ASSERT_NE(nullptr, result);
        EXPECT_NEAR(8.7, result->getValue(), 0.1);
    }

}

TEST_F(TransformerTest, ConstantPropagator) {
    {
        using namespace llvm;

        // 0.3 >= 20 && !(-5.4)
        auto testTerm = FN.Term->getCmpTerm(
            ConditionType::GE,
            FN.Term->getOpaqueConstantTerm(0.3),
            FN.Term->getBinaryTerm(
                ArithType::LAND,
                FN.Term->getConstTerm(
                    ConstantInt::get(*ctx, APInt(32, 20))
                ),
                FN.Term->getUnaryTerm(
                    UnaryArithType::NOT,
                    FN.Term->getConstTerm(
                        ConstantFP::get(*ctx, APFloat(5.4))
                    )
                )
            )
        );

        ConstantPropagator cp(FN);
        auto result = dyn_cast<OpaqueBoolConstantTerm>(cp.transform(testTerm));

        ASSERT_NE(nullptr, result);
        EXPECT_TRUE(result->getValue());
    }
}

} // namespace

/*
 * DefaultPredicateAnalysis.cpp
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

#include <vector>

#include "Codegen/llvm.h"
#include "Logging/tracer.hpp"
#include "Passes/PredicateAnalysis/DefaultPredicateAnalysis.h"
#include "Passes/SlotTrackerPass.h"

#include "Util/macros.h"

namespace borealis {

using borealis::util::tail;
using borealis::util::view;

////////////////////////////////////////////////////////////////////////////////
//
// Default predicate analysis instruction visitor
//
////////////////////////////////////////////////////////////////////////////////

class DPAInstVisitor : public llvm::InstVisitor<DPAInstVisitor> {

public:

    DPAInstVisitor(DefaultPredicateAnalysis* pass) : pass(pass) {}

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::Value;

        Value* lhv = &I;
        Value* rhv = I.getPointerOperand();

        pass->PM[&I] = pass->PF->getLoadPredicate(
                pass->TF->getValueTerm(lhv),
                pass->TF->getLoadTerm(
                        pass->TF->getValueTerm(rhv)
                )
        );
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using llvm::Value;

        Value* lhv = I.getPointerOperand();
        Value* rhv = I.getValueOperand();

        pass->PM[&I] = pass->PF->getStorePredicate(
                pass->TF->getValueTerm(lhv),
                pass->TF->getValueTerm(rhv)
        );
    }

    void visitICmpInst(llvm::ICmpInst& I) {
        using llvm::ConditionType;
        using llvm::Value;

        Value* lhv = &I;
        Value* op1 = I.getOperand(0);
        Value* op2 = I.getOperand(1);
        ConditionType cond = conditionType(I.getPredicate());

        pass->PM[&I] = pass->PF->getICmpPredicate(
                pass->TF->getValueTerm(lhv),
                pass->TF->getCmpTerm(
                        cond,
                        pass->TF->getValueTerm(op1),
                        pass->TF->getValueTerm(op2)
                )
        );
    }

    void visitBranchInst(llvm::BranchInst& I) {
        using llvm::BasicBlock;

        if (I.isUnconditional()) return;

        Term::Ptr condTerm = pass->TF->getValueTerm(I.getCondition());
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        pass->TPM[{&I, trueSucc}] =
                pass->PF->getBooleanPredicate(
                        condTerm,
                        pass->TF->getTrueTerm()
                );
        pass->TPM[{&I, falseSucc}] =
                pass->PF->getBooleanPredicate(
                        condTerm,
                        pass->TF->getFalseTerm()
                );
    }

    void visitSwitchInst(llvm::SwitchInst& I) {
        using llvm::BasicBlock;

        Term::Ptr condTerm = pass->TF->getValueTerm(I.getCondition());

        std::vector<Term::Ptr> cases;
        cases.reserve(I.getNumCases());

        for (auto c = I.case_begin(); c != I.case_end(); ++c) {
            Term::Ptr caseTerm = pass->TF->getConstTerm(c.getCaseValue());
            const BasicBlock* caseSucc = c.getCaseSuccessor();

            pass->TPM[{&I, caseSucc}] =
                    pass->PF->getEqualityPredicate(
                            condTerm,
                            caseTerm,
                            PredicateType::PATH
                    );

            cases.push_back(caseTerm);
        }

        const BasicBlock* defaultSucc = I.getDefaultDest();
        pass->TPM[{&I, defaultSucc}] =
                pass->PF->getDefaultSwitchCasePredicate(
                        condTerm,
                        cases
                );
    }

    void visitSelectInst(llvm::SelectInst& I) {
        using llvm::Value;

        Value* lhv = &I;
        Value* cnd = I.getCondition();
        Value* tru = I.getTrueValue();
        Value* fls = I.getFalseValue();

        pass->PM[&I] = pass->PF->getEqualityPredicate(
                pass->TF->getValueTerm(lhv),
                pass->TF->getTernaryTerm(
                        pass->TF->getValueTerm(cnd),
                        pass->TF->getValueTerm(tru),
                        pass->TF->getValueTerm(fls)
                )
        );
    }

    void visitAllocaInst(llvm::AllocaInst& I) {
        using namespace llvm;
        using llvm::Type;

        Value* lhv = &I;
        Value* arraySize = I.getArraySize();
        Type* allocatedType = I.getAllocatedType();

        ASSERTC(isa<ConstantInt>(arraySize));

        unsigned long long numElems = cast<ConstantInt>(arraySize)->getZExtValue();
        numElems = numElems * getTypeSizeInElems(allocatedType);

        ASSERTC(numElems > 0);

        pass->PM[&I] = pass->PF->getAllocaPredicate(
                pass->TF->getValueTerm(lhv),
                pass->TF->getIntTerm(numElems)
        );
    }

    void visitGetElementPtrInst(llvm::GetElementPtrInst& I) {
        using namespace llvm;

        Value* lhv = &I;
        Value* rhv = I.getPointerOperand();

        std::vector<llvm::Value*> idxs;
        idxs.reserve(I.getNumOperands() - 1);
        for (auto& i : tail(view(I.op_begin(), I.op_end()))) {
            idxs.push_back(i);
        }

        pass->PM[&I] = pass->PF->getEqualityPredicate(
                pass->TF->getValueTerm(lhv),
                pass->TF->getGepTerm(rhv, idxs)
        );
    }

    void visitCastInst(llvm::CastInst& I) {
        using namespace llvm;

        Value* lhv = &I;
        Value* rhv = I.getOperand(0);

        Term::Ptr lhvt = pass->TF->getValueTerm(lhv);
        Term::Ptr rhvt = pass->TF->getValueTerm(rhv);

        if (isa<Bool>(lhvt->getTermType()) && ! isa<Bool>(rhvt->getTermType())) {
            rhvt = pass->TF->getCmpTerm(
                    ConditionType::NEQ,
                    rhvt,
                    pass->TF->getIntTerm(0ULL)
            );
        } else if (! isa<Bool>(lhvt->getTermType()) && isa<Bool>(rhvt->getTermType())) {
            lhvt = pass->TF->getCmpTerm(
                    ConditionType::NEQ,
                    lhvt,
                    pass->TF->getIntTerm(0ULL)
            );
        }

        pass->PM[&I] = pass->PF->getEqualityPredicate(
                lhvt,
                rhvt
        );
    }

    void visitPHINode(llvm::PHINode& I) {
        using namespace llvm;

        for (unsigned int i = 0; i < I.getNumIncomingValues(); i++) {
            const BasicBlock* from = I.getIncomingBlock(i);
            Value* v = I.getIncomingValue(i);

            pass->PPM[{from, &I}] = pass->PF->getEqualityPredicate(
                    pass->TF->getValueTerm(&I),
                    pass->TF->getValueTerm(v)
            );
        }
    }

    void visitBinaryOperator(llvm::BinaryOperator& I) {
        using namespace llvm;

        typedef llvm::Instruction::BinaryOps OPS;

        Value* lhv = &I;
        Value* op1 = I.getOperand(0);
        Value* op2 = I.getOperand(1);
        ArithType type = arithType(I.getOpcode());

        pass->PM[&I] = pass->PF->getArithPredicate(
                pass->TF->getValueTerm(lhv),
                pass->TF->getBinaryTerm(
                        type,
                        pass->TF->getValueTerm(op1),
                        pass->TF->getValueTerm(op2)
                )
        );
    }

    void visitReturnInst(llvm::ReturnInst& I) {
        using llvm::Value;

        Value* rv = I.getReturnValue();
        if (rv == nullptr) return;

        pass->PM[&I] = pass->PF->getEqualityPredicate(
                pass->TF->getReturnValueTerm(I.getParent()->getParent()),
                pass->TF->getValueTerm(rv)
        );
    }

private:

    DefaultPredicateAnalysis* pass;

};

////////////////////////////////////////////////////////////////////////////////

DefaultPredicateAnalysis::DefaultPredicateAnalysis() :
        ProxyFunctionPass(ID) {}

DefaultPredicateAnalysis::DefaultPredicateAnalysis(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass) {}

void DefaultPredicateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<llvm::TargetData>::addRequiredTransitive(AU);
}

bool DefaultPredicateAnalysis::runOnFunction(llvm::Function& F) {
    init();

    auto* ST = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    PF = PredicateFactory::get(ST);
    TF = TermFactory::get(ST);

    TD = &GetAnalysis<llvm::TargetData>::doit(this, F);

    DPAInstVisitor visitor(this);
    visitor.visit(F);

    return false;
}

DefaultPredicateAnalysis::~DefaultPredicateAnalysis() {}

////////////////////////////////////////////////////////////////////////////////

char DefaultPredicateAnalysis::ID;
static RegisterPass<DefaultPredicateAnalysis>
X("default-predicate-analysis", "Default instruction predicate analysis");

} /* namespace borealis */

#include "Util/unmacros.h"

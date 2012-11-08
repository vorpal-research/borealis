/*
 * PredicateAnalysis.cpp
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#include "PredicateAnalysis.h"
#include "SlotTrackerPass.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////
//
// Predicate analysis instruction visitor
//
////////////////////////////////////////////////////////////////////////////////

class PredicateAnalysisInstVisitor :
        public llvm::InstVisitor<PredicateAnalysisInstVisitor> {

public:

    PredicateAnalysisInstVisitor(PredicateAnalysis* pass) : pass(pass) {}

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::Value;

        const Value* lhv = &I;
        const Value* rhv = I.getPointerOperand();

        pass->PM[&I] = pass->PF->getLoadPredicate(lhv, rhv);
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using llvm::Value;

        const Value* lhv = I.getPointerOperand();
        const Value* rhv = I.getValueOperand();

        pass->PM[&I] = pass->PF->getStorePredicate(lhv, rhv);
    }

    void visitICmpInst(llvm::ICmpInst& I) {
        using llvm::Value;

        const Value* lhv = &I;
        const Value* op1 = I.getOperand(0);
        const Value* op2 = I.getOperand(1);
        int pred = I.getPredicate();

        pass->PM[&I] = pass->PF->getICmpPredicate(lhv, op1, op2, pred);
    }

    void visitBranchInst(llvm::BranchInst& I) {
        using llvm::BasicBlock;
        using llvm::Value;

        if (I.isUnconditional()) return;

        const Value* cond = I.getCondition();
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        pass->TPM[std::make_pair(&I, trueSucc)] =
                pass->PF->getPathBooleanPredicate(cond, true);
        pass->TPM[std::make_pair(&I, falseSucc)] =
                pass->PF->getPathBooleanPredicate(cond, false);
    }

    void visitGetElementPtrInst(llvm::GetElementPtrInst& I) {
        using namespace llvm;
        using borealis::util::sayonara;

        Type* type = I.getPointerOperandType();

        std::vector< std::pair<const Value*, uint64_t> > shifts;
        for (auto it = I.idx_begin(); it != I.idx_end(); ++it) {
            const Value* v = *it;
            const uint64_t size = pass->TD->getTypeAllocSize(type);
            shifts.push_back(std::make_pair(v, size));

            if (type->isArrayTy()) type = type->getArrayElementType();
            else if (type->isStructTy()) {
                if (!isa<ConstantInt>(v)) {
                    sayonara(__FILE__, __LINE__,
                            "<PredicateAnalysisInstVisitor> Field shift not ConstantInt in visitGetElementPtrInst");
                }
                auto fieldIdx = cast<ConstantInt>(v)->getZExtValue();
                type = type->getStructElementType(fieldIdx);
            }
        }

        const Value* lhv = &I;
        const Value* rhv = I.getPointerOperand();

        pass->PM[&I] = pass->PF->getGEPPredicate(lhv, rhv, shifts);
    }

    void visitSExtInst(llvm::SExtInst& I) {
        using llvm::Value;

        const Value* lhv = &I;
        const Value* rhv = I.getOperand(0);

        pass->PM[&I] = pass->PF->getEqualityPredicate(lhv, rhv);
    }

    void visitBitCastInst(llvm::BitCastInst& I) {
        using llvm::Value;

        const Value* lhv = &I;
        const Value* rhv = I.getOperand(0);

        pass->PM[&I] = pass->PF->getEqualityPredicate(lhv, rhv);
    }

private:

    PredicateAnalysis* pass;

};

////////////////////////////////////////////////////////////////////////////////

PredicateAnalysis::PredicateAnalysis() : llvm::FunctionPass(ID) {}

void PredicateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
	Info.addRequiredTransitive<SlotTrackerPass>();
	Info.addRequiredTransitive<TargetData>();
}

bool PredicateAnalysis::runOnFunction(llvm::Function& F) {
	using namespace::llvm;

	init();

	PF = PredicateFactory::get(getAnalysis<SlotTrackerPass>().getSlotTracker(F));
    TD = &getAnalysis<TargetData>();

    PredicateAnalysisInstVisitor visitor(this);
    visitor.visit(F);

	return false;
}

PredicateAnalysis::~PredicateAnalysis() {
    for (const auto& entry : PM) {
        delete entry.second;
    }

    for (const auto& entry : TPM) {
        delete entry.second;
    }
}

char PredicateAnalysis::ID;
static llvm::RegisterPass<PredicateAnalysis>
X("predicate", "Instruction predicate analysis");

} /* namespace borealis */

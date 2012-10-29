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

    PredicateAnalysisInstVisitor(
            PredicateAnalysis::PredicateMap& PM,
            PredicateAnalysis::TerminatorPredicateMap& TPM,
            PredicateFactory* PF,
            TargetData* TD) : PM(PM), TPM(TPM), PF(PF), TD(TD) {
    }

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::Value;

        const Value* lhv = &I;
        const Value* rhv = I.getPointerOperand();

        PM[&I] = PF->getLoadPredicate(lhv, rhv);
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using llvm::Value;

        const Value* lhv = I.getPointerOperand();
        const Value* rhv = I.getValueOperand();

        PM[&I] = PF->getStorePredicate(lhv, rhv);
    }

    void visitICmpInst(llvm::ICmpInst& I) {
        using llvm::Value;

        const Value* lhv = &I;
        const Value* op1 = I.getOperand(0);
        const Value* op2 = I.getOperand(1);
        int pred = I.getPredicate();

        PM[&I] = PF->getICmpPredicate(lhv, op1, op2, pred);
    }

    void visitBranchInst(llvm::BranchInst& I) {
        using namespace::std;
        using llvm::BasicBlock;
        using llvm::Value;

        if (I.isUnconditional()) return;

        const Value* cond = I.getCondition();
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        TPM[make_pair(&I, trueSucc)] = PF->getPathBooleanPredicate(cond, true);
        TPM[make_pair(&I, falseSucc)] = PF->getPathBooleanPredicate(cond, false);
    }

    void visitGetElementPointerInst(llvm::GetElementPtrInst& I) {
        using namespace::std;
        using namespace::llvm;
        using borealis::util::sayonara;

        Type* type = I.getPointerOperandType();

        vector< pair<const Value*, uint64_t> > shifts;
        for (auto it = I.idx_begin(); it != I.idx_end(); ++it) {
            const Value* v = *it;
            const uint64_t size = TD->getTypeAllocSize(type);
            shifts.push_back(make_pair(v, size));

            if (type->isArrayTy()) type = type->getArrayElementType();
            else if (type->isStructTy()) {
                if (!isa<ConstantInt>(v)) {
                    sayonara(__FILE__, __LINE__,
                            "<GetElementPtrInstStruct> Field shift not ConstantInt");
                }
                auto fieldIdx = cast<ConstantInt>(v)->getZExtValue();
                type = type->getStructElementType(fieldIdx);
            }
        }

        const Value* lhv = &I;
        const Value* rhv = I.getPointerOperand();

        PM[&I] = PF->getGEPPredicate(lhv, rhv, shifts);
    }

    void visitSExtInst(llvm::SExtInst& I) {
        using llvm::Value;

        const Value* lhv = &I;
        const Value* rhv = I.getOperand(0);

        PM[&I] = PF->getEqualityPredicate(lhv, rhv);
    }

private:

    PredicateAnalysis::PredicateMap& PM;
    PredicateAnalysis::TerminatorPredicateMap& TPM;

    PredicateFactory* PF;
    TargetData* TD;

};

////////////////////////////////////////////////////////////////////////////////

PredicateAnalysis::PredicateAnalysis() : llvm::FunctionPass(ID) {
	// TODO
}

void PredicateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
	Info.addRequiredTransitive<SlotTrackerPass>();
	Info.addRequiredTransitive<TargetData>();
}

bool PredicateAnalysis::runOnFunction(llvm::Function& F) {
	using namespace::std;
	using namespace::llvm;

	init();

	PF = PredicateFactory::get(getAnalysis<SlotTrackerPass>().getSlotTracker(F));
    TD = &getAnalysis<TargetData>();

    PredicateAnalysisInstVisitor visitor(PM, TPM, PF.get(), TD);
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

} /* namespace borealis */

char borealis::PredicateAnalysis::ID;
static llvm::RegisterPass<borealis::PredicateAnalysis>
X("predicate", "Instruction predicate analysis", false, false);

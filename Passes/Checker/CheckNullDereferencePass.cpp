/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include <llvm/IR/InstVisitor.h>


#include "Util/disjoint_sets.hpp"

#include "Codegen/intrinsics_manager.h"
#include "Passes/Checker/CheckHelper.hpp"
#include "Passes/Checker/CheckNullDereferencePass.h"
#include "State/PredicateStateBuilder.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

static llvm::Value* stripAllOffsets(llvm::Value* base) {
    auto ptr = base->stripPointerCasts();
    if(auto gep = llvm::dyn_cast<llvm::GEPOperator>(base)) {
        return stripAllOffsets(gep->getPointerOperand());
    }
    return ptr;
}

static bool isNonNull(llvm::Value* ptr) {
    if(ptr->isDereferenceablePointer()) return true;

    auto&& im = IntrinsicsManager::getInstance();

    if(llvm::isa<llvm::GetElementPtrInst>(ptr)) return true;

    ptr = stripAllOffsets(ptr);

    if(llvm::isa<llvm::ConstantPointerNull>(ptr)) return false;
    if(llvm::is_one_of<llvm::Constant, llvm::AllocaInst>(ptr)) return true;

    if(auto call = llvm::ImmutableCallSite(ptr)) {
        if(auto f = call.getCalledFunction()) {
            if(f->getAttributes().hasAttribute(llvm::AttributeSet::ReturnIndex, llvm::Attribute::NonNull)) {
                return true;
            }
            if(im.getIntrinsicType(const_cast<llvm::Function*>(f)) == function_type::INTRINSIC_ALLOC) {
                return true;
            }
        }

    }
    return false;
}

class CheckNullsVisitor :
    public llvm::InstVisitor<CheckNullsVisitor>,
    // this is by design (i.e., sharing logging facilities)
    public borealis::logging::ClassLevelLogging<CheckNullDereferencePass> {

private:

    template<class Inst>
    void visitMemoryInst(Inst& I) {
        auto* ptr = I.getPointerOperand();
        if(isNonNull(ptr)) return;

        ptr = stripAllOffsets(ptr);

        if(auto&& iopt = util::at(p2i, ptr)) {
            CheckHelper<CheckNullDereferencePass> h(pass, &I, DefectType::INI_03);

            if (h.skip()) return;

            h.alias(iopt.getUnsafe());
        } else {
            p2i[ptr] = &I;

            CheckHelper<CheckNullDereferencePass> h(pass, &I, DefectType::INI_03);

            if (h.skip()) return;

            auto q = (
                pass->FN.State *
                pass->FN.Predicate->getInequalityPredicate(
                    pass->FN.Term->getValueTerm(ptr),
                    pass->FN.Term->getNullPtrTerm()
                )
            )();
            auto ps = pass->getInstructionState(&I);

            h.check(q, ps);
        }
    }

public:

    CheckNullsVisitor(CheckNullDereferencePass* pass) : pass(pass) {}

    void visitLoadInst(llvm::LoadInst& I) {
        visitMemoryInst(I);
    }

    void visitStoreInst(llvm::StoreInst& I) {
        visitMemoryInst(I);
    }

    void visitGetElementPtrInst(llvm::GetElementPtrInst& I) {
        visitMemoryInst(I);
    }


private:

    CheckNullDereferencePass* pass;
    std::unordered_map<llvm::Value*, llvm::Instruction*> p2i;

};

////////////////////////////////////////////////////////////////////////////////

CheckNullDereferencePass::CheckNullDereferencePass() : ProxyFunctionPass(ID) {}
CheckNullDereferencePass::CheckNullDereferencePass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void CheckNullDereferencePass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<CheckManager>::addRequiredTransitive(AU);

    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<llvm::AliasAnalysis>::addRequiredTransitive(AU);
    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<NameTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

bool CheckNullDereferencePass::runOnFunction(llvm::Function& F) {

    CM = &GetAnalysis<CheckManager>::doit(this, F);
    if (CM->shouldSkipFunction(&F)) return false;

    PSA = nullptr;
    //PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    AA = getAnalysisIfAvailable<llvm::AliasAnalysis>();
    DM = &GetAnalysis<DefectManager>::doit(this, F);
    FM = &GetAnalysis<FunctionManager>::doit(this, F);
    NT = &GetAnalysis<NameTracker>::doit(this, F);
    SLT = &GetAnalysis<SourceLocationTracker>::doit(this, F);

    ST = &GetAnalysis<SlotTrackerPass>::doit(this, F);
    FN = FactoryNest(F.getDataLayout(), ST->getSlotTracker(F));

    CheckNullsVisitor cnv(this);
    cnv.visit(F);

    DM->sync();
    return false;
}

PredicateState::Ptr CheckNullDereferencePass::getInstructionState(llvm::Instruction* I) {
    auto F = I->getParent()->getParent();
    if(!PSA) PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, *F);
    return PSA->getInstructionState(I);
}

CheckNullDereferencePass::~CheckNullDereferencePass() {}

char CheckNullDereferencePass::ID;
static RegisterPass<CheckNullDereferencePass>
X("check-null-deref", "NULL dereference checker");

} /* namespace borealis */

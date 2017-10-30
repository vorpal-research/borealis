/*
 * CheckContractPass.cpp
 *
 *  Created on: Feb 27, 2013
 *      Author: ice-phoenix
 */

#include <llvm/IR/InstVisitor.h>

#include "Codegen/intrinsics_manager.h"
#include "Passes/Checker/CheckContractPass.h"
#include "Passes/Checker/CheckHelper.hpp"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/AggregateTransformer.h"
#include "State/Transformer/AnnotationSubstitutor.h"
#include "State/Transformer/CallSiteInitializer.h"
#include "State/Transformer/ContractTransmogrifier.h"
#include "State/Transformer/Simplifier.h"

#include "Logging/tracer.hpp"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

template<class G>
static void popupGraph(G* g, bool wait = false) {
    static int pos = 0;
    ++pos;
    std::string realFileName = llvm::WriteGraph<G*>(g, "debug." + util::toString(pos), false, "Title");
    if (realFileName.empty()) return;
    llvm::DisplayGraph(realFileName, wait, llvm::GraphProgram::DOT);
}

class CallInstVisitor : public llvm::InstVisitor<CallInstVisitor> {

public:

    CallInstVisitor(CheckContractPass* pass) : pass(pass) {}

    void visitCallInst(llvm::CallInst& CI) {
        auto& im = IntrinsicsManager::getInstance();

        switch (im.getIntrinsicType(CI)) {
        case function_type::INTRINSIC_ANNOTATION:
            checkAnnotation(CI); break;
        case function_type::ACTION_DEFECT:
            checkActionDefect(CI); break;
        default:
            checkContract(CI);
            checkBonds(CI);
            break;
        }
    }

    void checkBonds(llvm::CallInst& CI) {
        for (auto& e : pass->FM->getBonds(CI.getCalledFunction())) {

            auto bond = e.second.first;
            auto defect = e.second.second;

            CheckHelper<CheckContractPass> h(pass, &CI, DefectType::UNK_99);

            if (h.skip(defect)) continue;

            auto t = Simplifier(pass->FN) +
                     ContractTransmogrifier(pass->FN) +
                     CallSiteInitializer(&CI, pass->FN);
            auto q = bond->map(
                [&t](Predicate::Ptr p) { return t.transform(p); }
            );
            auto ps = pass->getInstructionState(&CI);

            h.check(q, ps, defect);
        }
    }

    void checkContract(llvm::CallInst& CI) {

        CheckHelper<CheckContractPass> h(pass, &CI, DefectType::REQ_01);

        auto contract = pass->FM->getReq(CI, pass->FN);
        if(contract->isEmpty()) return;
        if (h.skip()) return;

        auto t = CallSiteInitializer(&CI, pass->FN);
        auto q = contract->map(
            [&t](Predicate::Ptr p) { return t.transform(p); }
        );

        auto ps = pass->getInstructionState(&CI);

        h.check(q, ps);
    }

    void checkActionDefect(llvm::CallInst& CI) {

        auto* op0 = CI.getArgOperand(0);
        auto defectType = getAsCompileTimeString(op0).getOrElse("UNK-99");
        auto defect = pass->DM->getDefect(defectType, &CI);

        CheckHelper<CheckContractPass> h(pass, &CI, DefectType::UNK_99);

        if (h.skip(defect)) return;

        auto state = pass->getInstructionState(&CI);
//        auto gr = buildGraphRep(state);
//        popupGraph(&gr, true);

        h.isReachable(state, defect);
    }

    void checkAnnotation(llvm::CallInst& CI) {
        auto anno = substituteAnnotationCall(pass->FN, llvm::CallSite(&CI));

        if (auto* LA = llvm::dyn_cast<AssertAnnotation>(anno)) {
            auto&& metaType = LA->getMeta();

            CheckHelper<CheckContractPass> h(pass, &CI, util::at(DefectTypesByName, metaType).getOrElse(DefectType::ASR_01));

            if (h.skip()) return;

            auto q = (
                pass->FN.State *
                pass->FN.Predicate->getEqualityPredicate(
                    LA->getTerm(),
                    pass->FN.Term->getTrueTerm()
                )
            )();
            auto ps = pass->getInstructionState(&CI);

            h.check(q, ps);
        }
    }

    void visitReturnInst(llvm::ReturnInst& RI) {

        CheckHelper<CheckContractPass> h(pass, &RI, DefectType::ENS_01);

        auto q = pass->FM->getEns(RI.getParent()->getParent());
        if(q->isEmpty()) return;
        if (h.skip()) return;

        auto ps = pass->getInstructionState(&RI);

        h.check(q, ps);
    }

private:

    CheckContractPass* pass;

};

////////////////////////////////////////////////////////////////////////////////

CheckContractPass::CheckContractPass() : ProxyFunctionPass(ID) {}
CheckContractPass::CheckContractPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void CheckContractPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<CheckManager>::addRequiredTransitive(AU);

    AUX<llvm::AliasAnalysis>::addRequiredTransitive(AU);
    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<VariableInfoTracker>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool CheckContractPass::runOnFunction(llvm::Function& F) {

    CM = &GetAnalysis<CheckManager>::doit(this, F);
    if (CM->shouldSkipFunction(&F)) return false;

    AA = getAnalysisIfAvailable<llvm::AliasAnalysis>();
    DM = &GetAnalysis<DefectManager>::doit(this, F);
    FM = &GetAnalysis<FunctionManager>::doit(this, F);
    MI = &GetAnalysis<VariableInfoTracker>::doit(this, F);
    // PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);
    PSA = nullptr;

    ST = &GetAnalysis<SlotTrackerPass>::doit(this, F);
    FN = FactoryNest(F.getDataLayout(), ST->getSlotTracker(F));

    CallInstVisitor civ(this);
    civ.visit(F);

    DM->sync();
    return false;
}

PredicateState::Ptr CheckContractPass::getInstructionState(llvm::Instruction* I) {
    auto F = I->getParent()->getParent();
    if(!PSA) PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, *F);
    return PSA->getInstructionState(I);
}

CheckContractPass::~CheckContractPass() {}

char CheckContractPass::ID;
static RegisterPass<CheckContractPass>
X("check-contracts", "Pass that checks annotated code contracts");

} /* namespace borealis */

/*
 * FunctionManager.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include "Annotation/LogicAnnotation.h"
#include "Codegen/intrinsics_manager.h"
#include "Passes/Manager/AnnotationManager.h"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/Tracker/MetaInfoTracker.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/AnnotationMaterializer.h"
#include "Util/passes.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

using borealis::util::view;

////////////////////////////////////////////////////////////////////////////////

FunctionManager::FunctionManager() : llvm::ModulePass(ID) {}

void FunctionManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<AnnotationManager>::addRequiredTransitive(AU);
    AUX<MetaInfoTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

bool FunctionManager::runOnModule(llvm::Module& M) {
    using namespace llvm;

    auto& annotations = GetAnalysis< AnnotationManager >::doit(this);
    auto& meta =  GetAnalysis< MetaInfoTracker >::doit(this);
    auto& locs = GetAnalysis< SourceLocationTracker >::doit(this);

    auto* st = GetAnalysis< SlotTrackerPass >::doit(this).getSlotTracker(M);
    FN = FactoryNest(st);

    unsigned int i = 0;
    for (auto* F : M) ids[F] = i++;

    for (auto a : annotations) {
        Annotation::Ptr anno = materialize(a, FN, &meta);
        if (auto* logic = dyn_cast<LogicAnnotation>(anno)) {

            if ( ! (isa<RequiresAnnotation>(logic) ||
                    isa<EnsuresAnnotation>(logic))) continue;

            for (auto& e : view(locs.getRangeFor(logic->getLocus()))) {
                if (auto* F = dyn_cast<Function>(e.second)) {
                    PredicateState::Ptr ps = (
                        FN.State *
                        FN.Predicate->getEqualityPredicate(
                            logic->getTerm(),
                            FN.Term->getTrueTerm(),
                            predicateType(logic)
                        )
                    )();
                    update(F, ps);
                    break;
                }
            }
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

void FunctionManager::put(llvm::Function* F, PredicateState::Ptr state) {
    using borealis::util::containsKey;

    ASSERT(!containsKey(data, F),
           "Attempt to register function " + F->getName().str() + " twice")

    data[F] = state;
}

void FunctionManager::update(llvm::Function* F, PredicateState::Ptr state) {
    using borealis::util::containsKey;

    dbgs() << "Updating function state for: " << F->getName().str() << endl
           << "  with: " << endl << state << endl;

    if (containsKey(data, F)) {
        data[F] = mergeFunctionDesc(data.at(F), state);
    } else {
        data[F] = state;
    }
}

////////////////////////////////////////////////////////////////////////////////

FunctionManager::FunctionDesc FunctionManager::get(llvm::Function* F) {
    using borealis::util::containsKey;

    if (containsKey(data, F)) {
        // Do nothing
    } else {
        data[F] = FN.State->Basic();
    }

    return data.at(F);
}

PredicateState::Ptr FunctionManager::getReq(llvm::Function* F) {
    const auto& desc = get(F);
    return desc.Req;
}

PredicateState::Ptr FunctionManager::getBdy(llvm::Function* F) {
    const auto& desc = get(F);
    return desc.Bdy;
}

PredicateState::Ptr FunctionManager::getEns(llvm::Function* F) {
    const auto& desc = get(F);
    return desc.Ens;
}

////////////////////////////////////////////////////////////////////////////////

FunctionManager::FunctionDesc FunctionManager::get(
        llvm::CallInst& CI,
        FactoryNest FN) {

    using borealis::util::containsKey;

    llvm::Function* F = CI.getCalledFunction();

    if (containsKey(data, F)) {
        return data.at(F);
    }

    auto& m = IntrinsicsManager::getInstance();
    function_type ft = m.getIntrinsicType(CI);

    auto state = FN.State->Basic();
    if (!isUnknown(ft)) {
        state = m.getPredicateState(ft, F, FN);
    }

    data[F] = state;
    return data.at(F);
}

PredicateState::Ptr FunctionManager::getReq(
        llvm::CallInst& CI,
        FactoryNest FN) {
    const auto& desc = get(CI, FN);
    return desc.Req;
}

PredicateState::Ptr FunctionManager::getBdy(
        llvm::CallInst& CI,
        FactoryNest FN) {
    const auto& desc = get(CI, FN);
    return desc.Bdy;
}

PredicateState::Ptr FunctionManager::getEns(
        llvm::CallInst& CI,
        FactoryNest FN) {
    const auto& desc = get(CI, FN);
    return desc.Ens;
}

unsigned int FunctionManager::getId(llvm::Function* F) const {
    ASSERTC(borealis::util::containsKey(ids, F));
    return ids.at(F);
}

unsigned int FunctionManager::getMemoryStart(llvm::Function* F) const {
    return (getId(F) << 16) + 1;
}

////////////////////////////////////////////////////////////////////////////////

FunctionManager::FunctionDesc FunctionManager::mergeFunctionDesc(const FunctionDesc& d1, const FunctionDesc& d2) const {
    return FunctionDesc{
        (FN.State * d1.Req + d2.Req)(),
        (FN.State * d1.Bdy + d2.Bdy)(),
        (FN.State * d1.Ens + d2.Ens)()
    };
}

////////////////////////////////////////////////////////////////////////////////

char FunctionManager::ID;
static RegisterPass<FunctionManager>
X("function-manager", "Pass that manages function analysis results");

} /* namespace borealis */

#include "Util/unmacros.h"

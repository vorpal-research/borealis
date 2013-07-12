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

    auto* slots = GetAnalysis< SlotTrackerPass >::doit(this).getSlotTracker(M);
    auto PF = PredicateFactory::get(slots);
    auto TF = TermFactory::get(slots);

    PSF = PredicateStateFactory::get();

    for (auto a : annotations) {
        Annotation::Ptr anno = materialize(a, TF.get(), &meta);
        if (auto* logic = dyn_cast<LogicAnnotation>(anno)) {

            if ( ! (isa<RequiresAnnotation>(logic) ||
                    isa<EnsuresAnnotation>(logic))) continue;

            for (auto& e : view(locs.getRangeFor(logic->getLocus()))) {
                if (auto* F = dyn_cast<Function>(e.second)) {
                    PredicateState::Ptr ps = (
                        PSF *
                        PF->getEqualityPredicate(
                            logic->getTerm(),
                            TF->getTrueTerm(),
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
        data[F] = PSF->Basic();
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
        PredicateFactory* PF,
        TermFactory* TF) {

    using borealis::util::containsKey;

    llvm::Function* F = CI.getCalledFunction();

    if (containsKey(data, F)) {
        return data.at(F);
    }

    auto& m = IntrinsicsManager::getInstance();
    function_type ft = m.getIntrinsicType(CI);

    auto state = PSF->Basic();
    if (!isUnknown(ft)) {
        state = m.getPredicateState(ft, F, PF, TF);
    }

    data[F] = state;
    return data.at(F);
}

PredicateState::Ptr FunctionManager::getReq(
        llvm::CallInst& CI,
        PredicateFactory* PF,
        TermFactory* TF) {
    const auto& desc = get(CI, PF, TF);
    return desc.Req;
}

PredicateState::Ptr FunctionManager::getBdy(
        llvm::CallInst& CI,
        PredicateFactory* PF,
        TermFactory* TF) {
    const auto& desc = get(CI, PF, TF);
    return desc.Bdy;
}

PredicateState::Ptr FunctionManager::getEns(
        llvm::CallInst& CI,
        PredicateFactory* PF,
        TermFactory* TF) {
    const auto& desc = get(CI, PF, TF);
    return desc.Ens;
}

////////////////////////////////////////////////////////////////////////////////

FunctionManager::FunctionDesc FunctionManager::mergeFunctionDesc(const FunctionDesc& d1, const FunctionDesc& d2) const {
    return FunctionDesc{
        (PSF * d1.Req + d2.Req)(),
        (PSF * d1.Bdy + d2.Bdy)(),
        (PSF * d1.Ens + d2.Ens)()
    };
}

////////////////////////////////////////////////////////////////////////////////

char FunctionManager::ID;
static RegisterPass<FunctionManager>
X("function-manager", "Pass that manages function analysis results");

} /* namespace borealis */

#include "Util/unmacros.h"

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
#include "Passes/Tracker/VariableInfoTracker.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/AnnotationMaterializer.h"
#include "State/Transformer/AnnotationSubstitutor.h"
#include "Util/passes.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

FunctionManager::FunctionManager() : llvm::ModulePass(ID) {}

void FunctionManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<AnnotationManager>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
    AUX<VariableInfoTracker>::addRequiredTransitive(AU);
}

bool FunctionManager::runOnModule(llvm::Module& M) {
    auto&& annotations = GetAnalysis< AnnotationManager >::doit(this);
    auto&& locs = GetAnalysis< SourceLocationTracker >::doit(this);
    auto&& meta =  GetAnalysis< VariableInfoTracker >::doit(this);

    FN = FactoryNest(GetAnalysis< SlotTrackerPass >::doit(this).getSlotTracker(M));

    unsigned int i = 1;
    for (auto&& F : M) ids[&F] = i++;

    for (auto&& a : annotations) {
        // FIXME: check this!!!
        auto&& anno = materialize(a, FN, &meta);
        if (auto* logic = llvm::dyn_cast<LogicAnnotation>(anno)) {

            if (not (llvm::isa<RequiresAnnotation>(logic) ||
                     llvm::isa<EnsuresAnnotation>(logic))) continue;

            for (auto&& e : util::view(locs.getRangeFor(logic->getLocus()))) {
                if (auto* F = llvm::dyn_cast<llvm::Function>(e.second)) {
                    auto&& ps = (
                        FN.State *
                        FN.Predicate->getEqualityPredicate(
                            logic->getTerm(),
                            FN.Term->getTrueTerm(),
                            e.first,
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

void FunctionManager::put(const llvm::Function* F, PredicateState::Ptr state) {
    ASSERT(not util::containsKey(data, F),
           "Attempt to register function " + F->getName().str() + " twice")

    data[F] = state;
}

void FunctionManager::update(const llvm::Function* F, PredicateState::Ptr state) {
    dbgs() << "Updating function state for: " << F->getName().str() << endl
           << "  with: " << endl << state << endl;

    if (util::containsKey(data, F)) {
        data[F] = mergeFunctionDesc(data.at(F), state);
    } else {
        data[F] = state;
    }
}

////////////////////////////////////////////////////////////////////////////////

FunctionManager::FunctionDesc FunctionManager::get(const llvm::Function* F) const {
    if (not util::containsKey(data, F)) {
        data[F] = FN.State->Basic();
    }

    return data.at(F);
}

PredicateState::Ptr FunctionManager::getReq(const llvm::Function* F) const {
    auto&& desc = get(F);
    return desc.Req;
}

PredicateState::Ptr FunctionManager::getBdy(const llvm::Function* F) const {
    auto&& desc = get(F);
    return desc.Bdy;
}

PredicateState::Ptr FunctionManager::getEns(const llvm::Function* F) const {
    auto&& desc = get(F);
    return desc.Ens;
}

////////////////////////////////////////////////////////////////////////////////

FunctionManager::FunctionDesc FunctionManager::get(
        const llvm::CallInst& CI,
        FactoryNest FN) const {

    auto* F = CI.getCalledFunction();

    if (util::containsKey(data, F)) {
        return data.at(F);
    }

    auto&& m = IntrinsicsManager::getInstance();
    auto&& ft = m.getIntrinsicType(CI);

    auto&& state = FN.State->Basic();
    if (not isUnknown(ft)) {
        state = m.getPredicateState(ft, F, FN);
    }

    data[F] = state;
    return data.at(F);
}

PredicateState::Ptr FunctionManager::getReq(
        const llvm::CallInst& CI,
        FactoryNest FN) const {
    auto&& desc = get(CI, FN);
    return desc.Req;
}

PredicateState::Ptr FunctionManager::getBdy(
        const llvm::CallInst& CI,
        FactoryNest FN) const {
    auto&& desc = get(CI, FN);
    return desc.Bdy;
}

PredicateState::Ptr FunctionManager::getEns(
        const llvm::CallInst& CI,
        FactoryNest FN) const {
    auto&& desc = get(CI, FN);
    return desc.Ens;
}

////////////////////////////////////////////////////////////////////////////////

unsigned int FunctionManager::getId(const llvm::Function* F) const {
    ASSERTC(util::containsKey(ids, F));
    return ids.at(F);
}

unsigned int FunctionManager::getMemoryStart(const llvm::Function* F) const {
    return (getId(F) << 16) + 1;
}

unsigned int FunctionManager::getMemoryEnd(const llvm::Function* F) const {
    return (getId(F) + 1) << 16;
}

std::pair<unsigned int, unsigned int> FunctionManager::getMemoryBounds(const llvm::Function* F) const {
    return { getMemoryStart(F), getMemoryEnd(F) };
}

////////////////////////////////////////////////////////////////////////////////

void FunctionManager::addBond(
        const llvm::Function* F,
        const Bond& bond) {
    bonds.insert({F, bond});
}

auto FunctionManager::getBonds(const llvm::Function* F) const -> decltype(util::view(bonds.equal_range(0))) {
    return util::view(bonds.equal_range(F));
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

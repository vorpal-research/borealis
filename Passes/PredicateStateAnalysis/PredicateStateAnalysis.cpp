/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include "Config/config.h"
#include "Passes/PredicateStateAnalysis/Defines.def"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"

#include "Util/macros.h"

namespace borealis {

AbstractPredicateStateAnalysis::AbstractPredicateStateAnalysis() {};
AbstractPredicateStateAnalysis::~AbstractPredicateStateAnalysis() {};

////////////////////////////////////////////////////////////////////////////////

PredicateStateAnalysis::PredicateStateAnalysis() :
        ProxyFunctionPass(ID) {}
PredicateStateAnalysis::PredicateStateAnalysis(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass) {}

void PredicateStateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    if ("one-for-one" == Mode())
        AUX<OneForOne>::addRequiredTransitive(AU);
    else if ("one-for-all" == Mode())
        AUX<OneForAll>::addRequiredTransitive(AU);
    else
        BYE_BYE_VOID("Unknown PSA mode: " + Mode());

    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<LocationManager>::addRequiredTransitive(AU);
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    if ("one-for-one" == Mode())
        delegate = &GetAnalysis<OneForOne>::doit(this, F);
    else if ("one-for-all" == Mode())
        delegate = &GetAnalysis<OneForAll>::doit(this, F);
    else
        BYE_BYE(bool, "Unknown PSA mode: " + Mode());

    updateVisitedLocs(F);

    if ("inline" == Summaries()) {
        updateInlineSummary(F);
    }

    return false;
}

void PredicateStateAnalysis::updateInlineSummary(llvm::Function& F) {
    // Update total function state
    // FIXME akhin Fix dep issues and remove manual update
    delegate->runOnFunction(F);

    // Save total function state to function manager
    // for inlining

    auto& FM = GetAnalysis<FunctionManager>::doit(this);

    auto initial = delegate->getInitialState();

    auto rets = getAllRets(&F);
    ASSERT(rets.size() <= 1,
           "Unexpected number of ReturnInst for: " + F.getName().str());

    // Function does not return, therefore has no useful summary
    if (rets.empty()) return;

    auto* RI = rets.front();

    auto riState = delegate->getInstructionState(RI);
    ASSERT(riState, "No state found for: " + llvm::valueSummary(RI));

    auto bdy = riState->sliceOn(initial);
    ASSERT(bdy, "Function state slicing failed for: " + llvm::valueSummary(RI));

    dbgs() << "Updating function state for: " << F.getName().str() << endl
           << "  with: " << endl << bdy << endl;

    FM.update(&F, bdy);
}

void PredicateStateAnalysis::updateVisitedLocs(llvm::Function& F) {
    delegate->runOnFunction(F);

    auto& LM = GetAnalysis<LocationManager>::doit(this);

    auto initial = delegate->getInitialState();

    auto rets = getAllRets(&F);
    ASSERT(rets.size() <= 1, "Unexpected number of ReturnInst for: " + F.getName().str());

    if (rets.empty()) return;
    auto RI = rets.front();
    auto riState = delegate->getInstructionState(RI);
    ASSERT(riState, "No state found for: " + llvm::valueSummary(RI));
    LM.addLocations(riState->getVisited());
}

void PredicateStateAnalysis::print(llvm::raw_ostream& O, const llvm::Module* M) const {
    delegate->printInstructionStates(O, M);
}

PredicateState::Ptr PredicateStateAnalysis::getInstructionState(const llvm::Instruction* I) const {
    return delegate->getInstructionState(I);
}

////////////////////////////////////////////////////////////////////////////////

char PredicateStateAnalysis::ID;
static RegisterPass<PredicateStateAnalysis>
X("predicate-state-analysis", "Delegating analysis that merges the results of separate predicate analyses");

const std::string PredicateStateAnalysis::Mode() {
    static config::ConfigEntry<std::string> mode("analysis", "psa-mode");
    return mode.get("one-for-one");
}

bool PredicateStateAnalysis::CheckUnreachable() {
    static config::ConfigEntry<bool> checkUnreachable("analysis", "psa-check-unreachable");
    return checkUnreachable.get(true);
}

const std::string PredicateStateAnalysis::Summaries() {
    static config::ConfigEntry<std::string> mode("analysis", "sum-mode");
    return mode.get("none");
}

} /* namespace borealis */


#include "Util/unmacros.h"

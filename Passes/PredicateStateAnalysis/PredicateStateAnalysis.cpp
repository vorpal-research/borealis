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
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    if ("one-for-one" == Mode())
        delegate = &GetAnalysis<OneForOne>::doit(this, F);
    else if ("one-for-all" == Mode())
        delegate = &GetAnalysis<OneForAll>::doit(this, F);
    else
        BYE_BYE(bool, "Unknown PSA mode: " + Mode());

    return false;
}

void PredicateStateAnalysis::print(llvm::raw_ostream& O, const llvm::Module* M) const {
    delegate->print(O, M);
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

} /* namespace borealis */

#include "Util/unmacros.h"

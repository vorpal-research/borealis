/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include "Passes/PredicateStateAnalysis.h"
#include "Passes/PredicateStateAnalysis/Defines.def"

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
    AUX<OneForAll>::addRequiredTransitive(AU);
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    delegate = &GetAnalysis<OneForAll>::doit(this, F);
    return false;
}

void PredicateStateAnalysis::print(llvm::raw_ostream& O, const llvm::Module* M) const {
    delegate->print(O, M);
}

const PredicateStateAnalysis::InstructionStates& PredicateStateAnalysis::getInstructionStates() const {
    return delegate->getInstructionStates();
}

////////////////////////////////////////////////////////////////////////////////

char PredicateStateAnalysis::ID;
static RegisterPass<PredicateStateAnalysis>
X("predicate-state-analysis", "Delegating analysis that merges the results of separate predicate analyses");

} /* namespace borealis */

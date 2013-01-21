/*
 * AbstractPredicateAnalysis.cpp
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#include "Passes/AbstractPredicateAnalysis.h"

namespace borealis {

AbstractPredicateAnalysis::RegisteredPasses
AbstractPredicateAnalysis::registeredPasses;

AbstractPredicateAnalysis::AbstractPredicateAnalysis(char ID) :
        llvm::FunctionPass(ID) {
    registeredPasses.insert((const void*)&ID);
};

AbstractPredicateAnalysis::~AbstractPredicateAnalysis() {}; // FIXME: akhin Remove ID on delete?

} // namespace borealis

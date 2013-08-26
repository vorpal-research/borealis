/*
 * LocationManager.cpp
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#include "Passes/Location/LocationManager.h"
#include "Passes/Tracker/SourceLocationTracker.h"

#include "Util/passes.hpp"

namespace borealis {

LocationManager::LocationManager(): llvm::ModulePass(ID) {}

void LocationManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

bool LocationManager::runOnModule(llvm::Module&) {
    return false;
}

void LocationManager::addLocations(const PredicateState::Locs& locs) {
    locs_.insert(locs.begin(), locs.end());
}

LocationManager::Locations LocationManager::locs_;

char LocationManager::ID;
static RegisterPass<LocationManager>
X("location-manager", "Pass that manages visited locations");

} /* namespace borealis */

/*
 * LocationManager.cpp
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#include "LocationManager.h"
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

char LocationManager::ID;
LocationManager::Locations LocationManager::locs_;
static RegisterPass<LocationManager>
X("location-manager", "Pass that manages locations which other passes visited");

} /* namespace borealis */


/*
 * LocationManager.cpp
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#include "Passes/Location/LocationManager.h"

#include "Util/passes.hpp"

namespace borealis {

LocationManager::LocationManager() : llvm::ImmutablePass(ID) {}

void LocationManager::addLocations(const LocationManager::Locations& loci_) {
    loci.insert(loci_.begin(), loci_.end());
}

const LocationManager::Locations& LocationManager::getLocations() const {
    return loci;
}

LocationManager::Locations LocationManager::loci;

char LocationManager::ID;
static RegisterPass<LocationManager>
X("location-manager", "Pass that manages visited locations");

} /* namespace borealis */

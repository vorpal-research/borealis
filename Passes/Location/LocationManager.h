/*
 * LocationManager.h
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#ifndef LOCATIONMANAGER_H_
#define LOCATIONMANAGER_H_

#include <llvm/IR/Value.h>
#include <llvm/Pass.h>

#include "State/PredicateState.h"

namespace borealis {

class LocationManager: public llvm::ImmutablePass {
public:
    using Locations = PredicateState::Loci;

    static char ID;

    LocationManager();
    virtual ~LocationManager() = default;

    void addLocations(const Locations& loci_);
    const Locations& getLocations() const;

private:
    static Locations loci;
};

} /* namespace borealis */

#endif /* LOCATIONMANAGER_H_ */

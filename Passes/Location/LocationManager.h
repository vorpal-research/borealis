/*
 * LocationManager.h
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#ifndef LOCATIONMANAGER_H_
#define LOCATIONMANAGER_H_

#include <llvm/Pass.h>
#include <llvm/Value.h>

#include "State/PredicateState.h"


namespace borealis {

class LocationManager: public llvm::ModulePass {
public:
    typedef PredicateState::Locs Locations;


    static char ID;

    LocationManager();
    virtual bool runOnModule(llvm::Module&) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage&) const override;
    virtual ~LocationManager() = default;

    void addLocations(const Locations&);
    const Locations& getLocations() const { return locs_; }

private:
    static Locations locs_;
};

} /* namespace borealis */
#endif /* LOCATIONMANAGER_H_ */

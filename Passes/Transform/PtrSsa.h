/*
 * PtrSSAPass.h
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#include "llvm/Pass.h"

#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Util/AggregateFunctionPass.hpp"
#include "Passes/Transform/PtrSsa/PhiInjectionPass.h"
#include "Passes/Transform/PtrSsa/SLInjectionPass.h"

namespace borealis {

class PtrSsa :
    public AggregateFunctionPass<
        ptrssa::PhiInjectionPass,
        ptrssa::StoreLoadInjectionPass
    > {

    typedef ptrssa::PhiInjectionPass phis_t;
    typedef ptrssa::StoreLoadInjectionPass sls_t;

    typedef AggregateFunctionPass< phis_t, sls_t > base;

public:

    static char ID;

    PtrSsa() : base(ID) {}

    virtual bool runOnFunction(llvm::Function& F) {
        auto& phis = getChildAnalysis<phis_t>();
        auto& sls = getChildAnalysis<sls_t>();

        phis.runOnFunction(F);
        sls.mergeOriginInfoFrom(phis);
        sls.runOnFunction(F);

        return false;
    }

    virtual ~PtrSsa() {};
};

} // namespace borealis

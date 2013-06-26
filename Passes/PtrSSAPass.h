/*
 * PtrSSAPass.h
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#include "llvm/Pass.h"

#include "Passes/SlotTrackerPass.h"
#include "Passes/Util/AggregateFunctionPass.hpp"
#include "PtrSSAPass/PhiInjectionPass.h"
#include "PtrSSAPass/SLInjectionPass.h"

namespace borealis {

class PtrSSAPass :
    public AggregateFunctionPass<
        ptrssa::PhiInjectionPass,
        ptrssa::StoreLoadInjectionPass
    > {

    typedef ptrssa::PhiInjectionPass phis_t;
    typedef ptrssa::StoreLoadInjectionPass sls_t;

    typedef AggregateFunctionPass< phis_t, sls_t > base;

public:

	static char ID;

	PtrSSAPass() : base(ID) {}

    virtual bool runOnFunction(llvm::Function& F) {
        auto& phis = getChildAnalysis<phis_t>();
        auto& sls = getChildAnalysis<sls_t>();

        phis.runOnFunction(F);
        sls.mergeOriginInfoFrom(phis);
        sls.runOnFunction(F);

        return false;
    }

    virtual ~PtrSSAPass() {};
};

} // namespace borealis

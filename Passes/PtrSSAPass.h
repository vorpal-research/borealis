/*
 * PtrSSAPass.h
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"

#include <algorithm>
#include <unordered_map>

#include "SlotTrackerPass.h"
#include "PtrSSAPass/PhiInjectionPass.h"
#include "PtrSSAPass/SLInjectionPass.h"

#include "Passes/Util/AggregateFunctionPass.hpp"

namespace borealis {

class PtrSSAPass : public AggregateFunctionPass<
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

    virtual ~PtrSSAPass(){};
};

} // namespace borealis

/*
 *	This pass creates new definitions for variables used as operands
 *	of specifific instructions: add, sub, mul, trunc.
 *
 *	These new definitions are inserted right after the use site, and
 *	all remaining uses dominated by this new definition are renamed
 *	properly.
*/


#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Constants.h"
#include <deque>
#include <algorithm>
#include <unordered_map>

#include "SlotTrackerPass.h"
#include "PtrSSAPass/PhiInjectionPass.h"
#include "PtrSSAPass/SLInjectionPass.h"

#include "AggregateFunctionPass.hpp"

namespace borealis {

using namespace llvm;
using namespace std;

class PtrSSAPass : public AggregateFunctionPass<
                                ptrssa::PhiInjectionPass,
                                ptrssa::StoreLoadInjectionPass
                          > {

    typedef AggregateFunctionPass<
            ptrssa::PhiInjectionPass,
            ptrssa::StoreLoadInjectionPass
      > base;

public:
	static char ID;
	PtrSSAPass() : base(ID) {}

    virtual bool runOnFunction(Function& F) {
        auto& phis = getChildAnalysis<ptrssa::PhiInjectionPass>();
        auto& sls = getChildAnalysis<ptrssa::StoreLoadInjectionPass>();

        phis.runOnFunction(F);

        sls.mergeOriginInfoFrom(phis);

        sls.runOnFunction(F);

        return false;
    }
};

}


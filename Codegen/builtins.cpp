/*
 * builtins.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Analysis/MemoryBuiltins.h>

#include "builtins.h"

#include "Util/util.h"

namespace borealis {

using borealis::util::sayonara;

builtin getBuiltInType(llvm::CallInst& CI) {
    using namespace llvm;

    if (isMalloc(&CI)) {
        return builtin::MALLOC;
    }

    return builtin::NOT_BUILTIN;
}

PredicateState getPredicateState(
        builtin bi,
        llvm::CallInst& CI,
        PredicateFactory* PF,
        TermFactory* TF) {

    llvm::Function* F = CI.getCalledFunction();

    switch (bi) {

    case builtin::MALLOC:
        {
            PredicateState res;
            res.addPredicate(
                    PF->getMallocPredicate(
                            TF->getReturnValueTerm(F)
                    )
            );
            return res;
        }

    default:
        {
            return sayonara<PredicateState>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                    "Unknown built-in type");
        }
    }
}

} // namespace borealis

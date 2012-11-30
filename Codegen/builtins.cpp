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

builtin getBuiltInType(llvm::Function& F) {
    using namespace llvm;

    if (isMalloc(&F)) {
        return builtin::MALLOC;
    }

    return builtin::NOT_BUILTIN;
}

PredicateState getPredicateState(
        builtin bi,
        llvm::Function* F,
        PredicateFactory* PF,
        TermFactory* TF) {
    using namespace borealis;

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

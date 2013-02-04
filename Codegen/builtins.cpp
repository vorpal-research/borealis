/*
 * builtins.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Analysis/MemoryBuiltins.h>

#include "Codegen/builtins.h"
#include "Util/util.h"

#include "Util/macros.h"

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
            return res.addPredicate(
                    PF->getMallocPredicate(
                            TF->getReturnValueTerm(F)
                    )
            );
        }

    default:
        {
            BYE_BYE(PredicateState, "Unknown built-in type");
        }
    }
}

} // namespace borealis

#include "Util/unmacros.h"

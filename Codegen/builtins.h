/*
 * builtins.h
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#ifndef BUILTINS_H_
#define BUILTINS_H_

#include <llvm/Function.h>
#include <llvm/Intrinsics.h>

#include "Predicate/PredicateFactory.h"
#include "State/PredicateState.h"
#include "Term/TermFactory.h"

namespace borealis {

enum class builtin {
    MALLOC,
    NOT_BUILTIN
};

builtin getBuiltInType(llvm::CallInst&);

PredicateState getPredicateState(builtin,
        llvm::CallInst&,
        PredicateFactory*,
        TermFactory*);

} // namespace borealis

#endif /* BUILTINS_H_ */

/*
 * intrinsics_registry.cpp
 *
 *  Created on: Feb 18, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Analysis/MemoryBuiltins.h>

#include "Codegen/intrinsics_manager.h"

namespace borealis {

static IntrinsicsManager::RegisterIntrinsic INTRINSIC_PTR_VERSION {
    function_type::INTRINSIC_PTR_VERSION,
    "ptrver",
    [](llvm::Function* F, PredicateFactory* PF, TermFactory* TF) {
        return PredicateState().addPredicate(
               PF->getEqualityPredicate(
                   TF->getReturnValueTerm(F),
                   TF->getArgumentTerm(&*F->arg_begin())
               )
       );
    }
};

static IntrinsicsManager::RegisterIntrinsic INTRINSIC_VALUE {
    function_type::INTRINSIC_VALUE,
    "value"
};

static IntrinsicsManager::RegisterIntrinsic INTRINSIC_GLOBAL_DESCRIPTOR_TABLE {
    function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
    "globals"
};

static IntrinsicsManager::RegisterIntrinsic INTRINSIC_GLOBAL {
    function_type::INTRINSIC_GLOBAL,
    "global"
};

static IntrinsicsManager::RegisterIntrinsic INTRINSIC_ANNOTATION {
    function_type::INTRINSIC_ANNOTATION,
    "annotation"
};

static IntrinsicsManager::RegisterIntrinsic BUILTIN_MALLOC {
    function_type::BUILTIN_MALLOC,
    "malloc",
    [](llvm::Function* F, PredicateFactory* PF, TermFactory* TF) {
        return PredicateState().addPredicate(
                PF->getMallocPredicate(
                        TF->getReturnValueTerm(F)
                )
        );
    },
    [](const IntrinsicsManager&, const llvm::CallInst& CI) {
        return llvm::isMalloc(&CI)
                ? function_type::BUILTIN_MALLOC
                : function_type::UNKNOWN;
    }
};

} // namespace borealis

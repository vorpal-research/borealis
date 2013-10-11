/*
 * intrinsics_registry.cpp
 *
 *  Created on: Feb 18, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Analysis/MemoryBuiltins.h>

#include "Codegen/intrinsics_manager.h"

#include "Util/macros.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpredefined-identifier-outside-function"

namespace borealis {

typedef IntrinsicsManager::RegisterIntrinsic RegisterIntrinsic;

std::string getCalledFunctionName(const llvm::CallInst& ci) {
    const llvm::Value* F = ci.getCalledFunction();
    F = F ? F : ci.getCalledValue();
    return F->hasName() ? F->getName() : "";
}

static RegisterIntrinsic INTRINSIC_PTR_VERSION {
    function_type::INTRINSIC_PTR_VERSION,
    "ptrver",
    [](
          llvm::Function* F,
          FactoryNest FN
      ) -> PredicateState::Ptr {
        ASSERTC(F->arg_size() > 0);
        return FN.State->Basic() +
                FN.Predicate->getEqualityPredicate(
                   FN.Term->getReturnValueTerm(F),
                   FN.Term->getArgumentTerm(F->arg_begin())
               );
    }
};

static RegisterIntrinsic INTRINSIC_VALUE {
    function_type::INTRINSIC_VALUE,
    "value"
};

static RegisterIntrinsic INTRINSIC_DECLARE {
    function_type::INTRINSIC_DECLARE,
    "declare"
};

static RegisterIntrinsic INTRINSIC_GLOBAL_DESCRIPTOR_TABLE {
    function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
    "globals"
};

static RegisterIntrinsic INTRINSIC_GLOBAL {
    function_type::INTRINSIC_GLOBAL,
    "global"
};

static RegisterIntrinsic INTRINSIC_ANNOTATION {
    function_type::INTRINSIC_ANNOTATION,
    "annotation"
};

static RegisterIntrinsic INTRINSIC_MALLOC {
    function_type::INTRINSIC_MALLOC,
    "malloc",
    [](
          llvm::Function* F,
          FactoryNest FN
      ) -> PredicateState::Ptr {
        ASSERTC(F->arg_size() > 0);
        return FN.State->Basic() +
               FN.Predicate->getMallocPredicate(
                   FN.Term->getReturnValueTerm(F),
                   FN.Term->getArgumentTerm(F->arg_begin())
               );
    }
};

static RegisterIntrinsic BUILTIN_BOR_ASSERT {
    function_type::BUILTIN_BOR_ASSERT,
    "borealis_assert",
    [](
          llvm::Function* F,
          FactoryNest FN
      ) -> PredicateState::Ptr {
        ASSERTC(F->arg_size() > 0);
        return FN.State->Basic() +
               FN.Predicate->getInequalityPredicate(
                   FN.Term->getArgumentTerm(F->arg_begin()),
                   FN.Term->getIntTerm(0ULL),
                   PredicateType::REQUIRES
               );
    },
    [](const IntrinsicsManager&, const llvm::CallInst& ci) -> function_type {
        auto name = getCalledFunctionName(ci);
        return name == "borealis_assert"
               ? function_type::BUILTIN_BOR_ASSERT
               : function_type::UNKNOWN;
    }
};

static RegisterIntrinsic BUILTIN_BOR_ASSUME {
    function_type::BUILTIN_BOR_ASSUME,
    "borealis_assume",
    [](
          llvm::Function* F,
          FactoryNest FN
      ) -> PredicateState::Ptr {
        ASSERTC(F->arg_size() > 0);
        return FN.State->Basic() +
               FN.Predicate->getInequalityPredicate(
                   FN.Term->getArgumentTerm(F->arg_begin()),
                   FN.Term->getIntTerm(0ULL),
                   PredicateType::ENSURES
               );
    },
    [](const IntrinsicsManager&, const llvm::CallInst& ci) -> function_type {
        auto name = getCalledFunctionName(ci);
        return name == "borealis_assume"
               ? function_type::BUILTIN_BOR_ASSUME
               : function_type::UNKNOWN;
    }
};

static RegisterIntrinsic ACTION_DEFECT {
    function_type::ACTION_DEFECT,
    "borealis_action_defect",
    RegisterIntrinsic::DefaultGenerator,
    [](const IntrinsicsManager&, const llvm::CallInst& ci) -> function_type {
        auto name = getCalledFunctionName(ci);
        return name == "borealis_action_defect"
               ? function_type::ACTION_DEFECT
               : function_type::UNKNOWN;
    }
};

static RegisterIntrinsic BUILTIN_BOR_GETPROP {
    function_type::BUILTIN_BOR_GETPROP,
    "borealis_get_property",
    [](
          llvm::Function* F,
          FactoryNest FN
      ) -> PredicateState::Ptr {
        ASSERTC(F->arg_size() > 1);

        auto it = F->arg_begin();
        llvm::Argument* propName = it++;
        llvm::Argument* ptr = it++;

        return FN.State->Basic() +
               FN.Predicate->getEqualityPredicate(
                   FN.Term->getReturnValueTerm(F),
                   FN.Term->getReadPropertyTerm(
                       FN.Type->cast(F->getReturnType()),
                       FN.Term->getStringArgumentTerm(propName),
                       FN.Term->getArgumentTerm(ptr)
                   )
               );
    },
    [](const IntrinsicsManager&, const llvm::CallInst& ci) -> function_type {
        auto name = getCalledFunctionName(ci);
        return name == "borealis_get_property"
               ? function_type::BUILTIN_BOR_GETPROP
               : function_type::UNKNOWN;
    }
};

static RegisterIntrinsic BUILTIN_BOR_SETPROP {
    function_type::BUILTIN_BOR_SETPROP,
    "borealis_set_property",
    [](
          llvm::Function* F,
          FactoryNest FN
      ) -> PredicateState::Ptr {
        ASSERTC(F->arg_size() > 2);

        auto it = F->arg_begin();
        llvm::Argument* propName = it++;
        llvm::Argument* ptr = it++;
        llvm::Argument* value = it++;

        return FN.State->Basic() +
               FN.Predicate->getWritePropertyPredicate(
                   FN.Term->getStringArgumentTerm(propName),
                   FN.Term->getArgumentTerm(ptr),
                   FN.Term->getArgumentTerm(value)
               );
    },
    [](const IntrinsicsManager&, const llvm::CallInst& ci) -> function_type {
        auto name = getCalledFunctionName(ci);
        return name == "borealis_set_property"
               ? function_type::BUILTIN_BOR_SETPROP
               : function_type::UNKNOWN;
    }
};

} // namespace borealis

#pragma clang diagnostic pop

#include "Util/unmacros.h"

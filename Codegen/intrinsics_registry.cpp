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

static llvm::StringRef getFunctionName(const llvm::Function& F) {
    return F.hasName() ? F.getName() : "";
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
        ASSERTC(F->arg_size() > 2);

        auto it = F->arg_begin();
        auto resolvedSize = it++;
        auto origElemSize = it++;
        auto origNumElems = it++;
        auto origElemSizeTerm = FN.Term->getArgumentTerm(origElemSize);
        auto origNumElemsTerm = FN.Term->getArgumentTerm(origNumElems);
        auto newNumElems = FN.Term->getCastTerm(origElemSizeTerm->getType(), false, origNumElemsTerm);

        return FN.State->Basic() +
               FN.Predicate->getMallocPredicate(
                   FN.Term->getReturnValueTerm(F),
                   FN.Term->getArgumentTerm(resolvedSize),
                   FN.Term->getBinaryTerm(
                       llvm::ArithType::MUL,
                       origElemSizeTerm,
                       newNumElems
                   )
               );
    }
};

static RegisterIntrinsic INTRINSIC_ALLOC {
    function_type::INTRINSIC_ALLOC,
    "alloc",
    [](
          llvm::Function* F,
          FactoryNest FN
      ) -> PredicateState::Ptr {
        ASSERTC(F->arg_size() > 2);

        auto it = F->arg_begin();
        auto resolvedSize = it++;
        auto origElemSize = it++;
        auto origNumElems = it++;
        auto origElemSizeTerm = FN.Term->getArgumentTerm(origElemSize);
        auto origNumElemsTerm = FN.Term->getArgumentTerm(origNumElems);
        auto newNumElems = FN.Term->getCastTerm(origElemSizeTerm->getType(), false, origNumElemsTerm);

        return FN.State->Basic() +
               FN.Predicate->getAllocaPredicate(
                   FN.Term->getReturnValueTerm(F),
                   FN.Term->getArgumentTerm(resolvedSize),
                   FN.Term->getBinaryTerm(
                       llvm::ArithType::MUL,
                       origElemSizeTerm,
                       newNumElems
                   )
               );
    }
};

static RegisterIntrinsic INTRINSIC_NONDET {
    function_type::INTRINSIC_NONDET,
    "nondet"
};

static RegisterIntrinsic INTRINSIC_CONSUME {
    function_type::INTRINSIC_CONSUME,
    "consume"
};

static RegisterIntrinsic INTRINSIC_UNREACHABLE {
    function_type::INTRINSIC_UNREACHABLE,
    "unreachable"
};

static RegisterIntrinsic INTRINSIC_CALL_AND_STORE {
    function_type::INTRINSIC_CALL_AND_STORE,
    "call_and_store"
};

static RegisterIntrinsic BUILTIN_BOR_ASSERT {
    function_type::BUILTIN_BOR_ASSERT,
    "borealis_assert",
    [](
          llvm::Function* F,
          FactoryNest FN
      ) -> PredicateState::Ptr {
        ASSERTC(F->arg_size() > 0);
        if (F->arg_begin()->getType()->isIntegerTy(1)) {
            return FN.State->Basic() +
                   FN.Predicate->getEqualityPredicate(
                       FN.Term->getArgumentTerm(F->arg_begin()),
                       FN.Term->getTrueTerm(),
                       Locus(),
                       PredicateType::REQUIRES
                   );
        } else {
            return FN.State->Basic() +
                   FN.Predicate->getInequalityPredicate(
                       FN.Term->getArgumentTerm(F->arg_begin()),
                       FN.Term->getIntTerm(0, FN.Type->cast(F->arg_begin()->getType(), F->getParent()->getDataLayout())),
                       Locus(),
                       PredicateType::REQUIRES
                   );
        }
    },
    [](const IntrinsicsManager&, const llvm::Function& f) -> function_type {
        auto name = getFunctionName(f);

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
        if (F->arg_begin()->getType()->isIntegerTy(1)) {
            return FN.State->Basic() +
                   FN.Predicate->getEqualityPredicate(
                       FN.Term->getArgumentTerm(F->arg_begin()),
                       FN.Term->getTrueTerm(),
                       Locus(),
                       PredicateType::ENSURES
                   );
        } else {
            return FN.State->Basic() +
                   FN.Predicate->getInequalityPredicate(
                       FN.Term->getArgumentTerm(F->arg_begin()),
                       FN.Term->getIntTerm(0, FN.Type->cast(F->arg_begin()->getType(), F->getParent()->getDataLayout())),
                       Locus(),
                       PredicateType::ENSURES
                   );
        }
    },
    [](const IntrinsicsManager&, const llvm::Function& f) -> function_type {
        auto name = getFunctionName(f);
        return name == "borealis_assume"
               ? function_type::BUILTIN_BOR_ASSUME
               : function_type::UNKNOWN;
    }
};

static RegisterIntrinsic ACTION_DEFECT {
    function_type::ACTION_DEFECT,
    "borealis_action_defect",
    RegisterIntrinsic::DefaultGenerator,
    [](const IntrinsicsManager&, const llvm::Function& f) -> function_type {
        auto name = getFunctionName(f);
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
                       FN.Type->cast(F->getReturnType(), F->getParent()->getDataLayout()),
                       FN.Term->getStringArgumentTerm(propName),
                       FN.Term->getArgumentTerm(ptr)
                   )
               );
    },
    [](const IntrinsicsManager&, const llvm::Function& f) -> function_type {
        auto name = getFunctionName(f);
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

        auto ptrt = FN.Term->getArgumentTerm(ptr);
        if(not llvm::isa<type::Pointer>(ptrt->getType())) {
            ptrt = FN.Term->getCastTerm(FN.Type->getPointer(FN.Type->getUnknownType()), false, ptrt);
        }

        return FN.State->Basic() +
               FN.Predicate->getWritePropertyPredicate(
                   FN.Term->getStringArgumentTerm(propName),
                   ptrt,
                   FN.Term->getArgumentTerm(value)
               );
    },
    [](const IntrinsicsManager&, const llvm::Function& f) -> function_type {
        auto name = getFunctionName(f);
        return name == "borealis_set_property"
               ? function_type::BUILTIN_BOR_SETPROP
               : function_type::UNKNOWN;
    }
};

} // namespace borealis

#pragma clang diagnostic pop

#include "Util/unmacros.h"

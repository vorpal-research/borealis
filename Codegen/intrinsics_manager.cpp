/*
 * intrinsics_manager.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Analysis/MemoryBuiltins.h>

#include "Codegen/intrinsics_manager.h"
#include "Util/macros.h"

namespace borealis {

llvm::Function* IntrinsicsManager::createIntrinsic(
        function_type ft,
        const std::string& ext,
        llvm::FunctionType* funtype,
        llvm::Module* module) {
    using llvm::GlobalValue;
    using llvm::Function;
    using borealis::util::containsKey;

    if (containsKey(intrinsics_cache, {ft, funtype})) {
        return intrinsics_cache[{ft, funtype}];
    }

    Function* f = Function::Create(
            funtype,
            GlobalValue::ExternalLinkage,
            getFuncName(ft, ext),
            module
    );

    type_cache[f] = ft;
    intrinsics_cache[{ft, funtype}] = f;

    return f;
}

function_type IntrinsicsManager::getIntrinsicType(llvm::CallInst& CI) const {
    for (auto& resolver : resolvers) {
        function_type r = resolver(*this, CI);
        if (r != function_type::UNKNOWN) return r;
    }
    return function_type::UNKNOWN;
}

PredicateState IntrinsicsManager::getPredicateState(
        function_type ft,
        llvm::Function* F,
        PredicateFactory* PF,
        TermFactory* TF) const {
    return getIntrinsicInfo(ft).generator(F, PF, TF);
}

IntrinsicsManager::IntrinsicInfo IntrinsicsManager::getIntrinsicInfo(function_type ft) const {
    using borealis::util::containsKey;
    if (containsKey(info_cache, ft)) {
        return info_cache.at(ft);
    } else {
        BYE_BYE(IntrinsicsManager::IntrinsicInfo, "Unknown function type");
    }
}

const std::string IntrinsicsManager::getFuncName(function_type ft, const std::string& ext) const {
    std::string buf;
    llvm::raw_string_ostream oss(buf);

    oss << "borealis." << getIntrinsicInfo(ft).name << "." << ext;
    return oss.str();
}

function_type IntrinsicsManager::getIntrinsicType(llvm::Function* F) const {
    auto iter = type_cache.find(F);
    if (iter != type_cache.end()) {
        return (*iter).second;
    } else {
        return function_type::UNKNOWN;
    }
}

////////////////////////////////////////////////////////////////////////////////

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
    "value",
    [](llvm::Function*, PredicateFactory*, TermFactory*) {
        return PredicateState();
    }
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
    [](const IntrinsicsManager&, llvm::CallInst& CI) {
        return llvm::isMalloc(&CI)
                ? function_type::BUILTIN_MALLOC
                : function_type::UNKNOWN;
    }
};

////////////////////////////////////////////////////////////////////////////////

} // namespace borealis

#include "Util/unmacros.h"

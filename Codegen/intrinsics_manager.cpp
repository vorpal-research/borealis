/*
 * intrinsics_manager.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include "Codegen/intrinsics_manager.h"

#include "Util/macros.h"

namespace borealis {

llvm::Function* IntrinsicsManager::createIntrinsic(
        function_type ft,
        const std::string& ext,
        llvm::FunctionType* funtype,
        llvm::Module* module) {
    using borealis::util::containsKey;

    if (containsKey(intrinsics_cache, {ft, funtype})) {
        return intrinsics_cache.at({ft, funtype});
    }

    auto* f = llvm::Function::Create(
            funtype,
            llvm::GlobalValue::ExternalLinkage,
            getFuncName(ft, ext),
            module
    );

    intrinsics_cache[{ft, funtype}] = f;
    type_cache[f] = ft;

    return f;
}

function_type IntrinsicsManager::getIntrinsicType(const llvm::CallInst& CI) const {
    for (auto& resolver : resolvers) {
        function_type r = resolver(*this, CI);
        if (r != function_type::UNKNOWN) return r;
    }
    return function_type::UNKNOWN;
}

PredicateState::Ptr IntrinsicsManager::getPredicateState(
        function_type ft,
        llvm::Function* F,
        PredicateFactory* PF,
        TermFactory* TF) const {
    return getIntrinsicInfo(ft).generator(F, PF, TF);
}

const std::string IntrinsicsManager::getFuncName(function_type ft, const std::string& ext) const {
    std::string buf;
    std::ostringstream oss(buf);

    oss << "borealis." << getIntrinsicInfo(ft).name;
    if (!ext.empty()) oss << "." << ext;
    return oss.str();
}

function_type IntrinsicsManager::getIntrinsicType(llvm::Function* F) const {
    using borealis::util::containsKey;
    if (containsKey(type_cache, F)) {
        return type_cache.at(F);
    }

    return function_type::UNKNOWN;
}

IntrinsicsManager::IntrinsicInfo IntrinsicsManager::getIntrinsicInfo(function_type ft) const {
    using borealis::util::containsKey;
    if (containsKey(info_cache, ft)) {
        return info_cache.at(ft);
    }

    BYE_BYE(IntrinsicsManager::IntrinsicInfo, "Unknown function type");
}

} // namespace borealis

#include "Util/unmacros.h"

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
    auto key = std::make_tuple(ft, funtype, module);

    if (containsKey(intrinsics_cache, key)) {
        return intrinsics_cache.at(key);
    }

    auto* f = llvm::Function::Create(
        funtype,
        llvm::GlobalValue::ExternalLinkage,
        getFuncName(ft, ext),
        module
    );

    intrinsics_cache[key] = f;
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
        FactoryNest FN) const {
    return getIntrinsicInfo(ft).generator(F, FN);
}

const std::string IntrinsicsManager::getFuncName(function_type ft, const std::string& ext) const {
    std::string buf;
    std::ostringstream oss(buf);

    if(ft >= function_type::FIRST_INTRINSIC && ft <= function_type::LAST_INTRINSIC) {
        oss << "borealis.";
    }

    oss << getIntrinsicInfo(ft).name;
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

void IntrinsicsManager::updateForModule(llvm::Module& M) {
    type_cache.clear();
    intrinsics_cache.clear();

    for (llvm::Function& F : M) {
        llvm::StringRef borealis, name;
        std::tie(borealis, name) = F.getName().split('.');
        std::tie(name, std::ignore) = name.split('.');
        if (borealis == "borealis" && !name.empty()) {
            for (auto type : util::at(name_cache, name)) {
                type_cache[&F] = type;
                intrinsics_cache[std::make_tuple(type, F.getFunctionType(), &M)] = &F;
            }
        }
    }
}

} // namespace borealis

#include "Util/unmacros.h"

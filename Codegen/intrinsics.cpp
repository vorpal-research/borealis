/*
 * intrinsics.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include <unordered_map>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Type.h>

#include "intrinsics.h"

using namespace borealis;

const std::string borealis::getFuncName(intrinsic intr, llvm::Type* type) {
    static std::unordered_map<intrinsic, std::string> names {
        { intrinsic::PTR_VERSION, "ptrver" },
    };

    std::string buf;
    llvm::raw_string_ostream oss(buf);

    oss << "borealis." << names[intr] << "." << *type;
    return oss.str();
}

llvm::Function* borealis::createIntrinsic(
        intrinsic intr,
        llvm::Module* module,
        llvm::FunctionType* funtype,
        llvm::Type* param) {
    using llvm::GlobalValue;
    using llvm::Function;

    return Function::Create(
            funtype,
            GlobalValue::ExternalLinkage,
            getFuncName(intr, param),
            module
    );
}

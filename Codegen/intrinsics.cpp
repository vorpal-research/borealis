/*
 * intrinsics.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include "intrinsics.h"

#include <unordered_map>

using namespace borealis;
using std::unordered_map;

#include <llvm/Module.h>
using llvm::Module;
#include <llvm/Function.h>
using llvm::Function;
using llvm::FunctionType;
#include <llvm/Type.h>
using llvm::Type;
#include <llvm/Support/raw_ostream.h>
using llvm::raw_string_ostream;

const string borealis::getFuncName(intrinsic intr, llvm::Type* type) {
    static unordered_map<intrinsic, string> names {
        { intrinsic::PTR_VERSION, "ptrver" },
    };

    string buf;
    llvm::raw_string_ostream oss(buf);

    oss << "borealis." << names[intr] << "." << *type;
    return oss.str();
}

Function* borealis::createIntrinsic(
        intrinsic intr,
        Module* module,
        FunctionType* funtype,
        Type* param) {
    using llvm::GlobalValue;

    return Function::Create(
            funtype,
            GlobalValue::ExternalLinkage,
            getFuncName(intr, param),
            module
    );
}

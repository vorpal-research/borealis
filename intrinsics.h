/*
 * intrinsics.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef INTRINSICS_H_
#define INTRINSICS_H_

#include <string>
#include <unordered_map>

#include <llvm/Type.h>
#include <llvm/Function.h>
#include <llvm/Module.h>

#include "util.h"

namespace borealis {

using std::string;

enum class intrinsic {
    PTR_VERSION,
};

const string getFuncName(intrinsic, llvm::Type*);
llvm::Function* createIntrinsic(
        intrinsic, llvm::Module*, llvm::FunctionType*, llvm::Type*
);

typedef std::unordered_map<llvm::Type*, llvm::Function*> typed_intrinsics_cache;

}

namespace std {

template<>
struct hash<borealis::intrinsic>: public borealis::util::enum_hash<borealis::intrinsic> {};

}


#endif /* INTRINSICS_H_ */

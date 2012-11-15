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

#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Type.h>

#include "Util/util.h"

namespace borealis {

enum class intrinsic {
    PTR_VERSION,
    NOT_INTRINSIC
};

const std::string getFuncName(intrinsic, llvm::Type*);

typedef std::unordered_map<llvm::Type*, llvm::Function*> typed_intrinsics_cache;

}

namespace std {

template<>
struct hash<borealis::intrinsic>: public borealis::util::enums::enum_hash<borealis::intrinsic> {};

}

#endif /* INTRINSICS_H_ */

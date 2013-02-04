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
#include <llvm/Value.h>

#include "Predicate/PredicateFactory.h"
#include "State/PredicateState.h"
#include "Term/TermFactory.h"
#include "Util/util.h"

namespace borealis {

enum class intrinsic {
    PTR_VERSION,
    NOT_INTRINSIC
};

const std::string getFuncName(intrinsic, llvm::Type*);

PredicateState getPredicateState(intrinsic, llvm::Function*, PredicateFactory*, TermFactory*);

typedef std::unordered_map<llvm::Type*, llvm::Function*> typed_intrinsics_cache;

} // namespace borealis

namespace std {

template<>
struct hash<borealis::intrinsic> : public borealis::util::enums::enum_hash<borealis::intrinsic> {};

} // namespace std

#endif /* INTRINSICS_H_ */

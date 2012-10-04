/*
 * FalsePredicate.cpp
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#include "FalsePredicate.h"

namespace borealis {

FalsePredicate::FalsePredicate(const llvm::Value* v, SlotTracker* st):
	v(v),
	vs(st->getLocalName(v)),
	asString(vs + "=FALSE") {
}

std::string FalsePredicate::toString() const {
	return asString;
}

Predicate::Key FalsePredicate::getKey() const {
	return std::make_pair(std::type_index(typeid(*this)).hash_code(), v);
}

} /* namespace borealis */

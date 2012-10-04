/*
 * TruePredicate.cpp
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#include "TruePredicate.h"

namespace borealis {

TruePredicate::TruePredicate(const llvm::Value* v, SlotTracker* st):
	v(v),
	vs(st->getLocalName(v)),
	asString(vs + "=TRUE") {
}

std::string TruePredicate::toString() const {
	return asString;
}

Predicate::Key TruePredicate::getKey() const {
	return std::make_pair(std::type_index(typeid(*this)).hash_code(), v);
}

} /* namespace borealis */

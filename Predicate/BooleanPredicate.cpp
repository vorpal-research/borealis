/*
 * BooleanPredicate.cpp
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#include "BooleanPredicate.h"

namespace borealis {

BooleanPredicate::BooleanPredicate(
		const llvm::Value* v,
		const bool b,
		SlotTracker* st):
	v(v),
	vs(st->getLocalName(v)),
	b(b),
	bs(b ? "TRUE" : "FALSE"),
	asString(vs + "=" + bs) {
}

std::string BooleanPredicate::toString() const {
	return asString;
}

Predicate::Key BooleanPredicate::getKey() const {
	return std::make_pair(std::type_index(typeid(*this)).hash_code(), v);
}

} /* namespace borealis */

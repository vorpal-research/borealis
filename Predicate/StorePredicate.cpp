/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "StorePredicate.h"

namespace borealis {

StorePredicate::StorePredicate(
		const llvm::Value* lhv,
		const llvm::Value* rhv,
		SlotTracker* st) :
				lhv(lhv),
				rhv(rhv),
				lhvs(st->getLocalName(lhv)),
				rhvs(st->getLocalName(rhv)),
				asString(lhvs + "=*" + rhvs){
}

std::string StorePredicate::toString() const {
	return asString;
}

} /* namespace borealis */

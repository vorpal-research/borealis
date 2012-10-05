/*
 * FalsePredicate.h
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#ifndef FALSEPREDICATE_H_
#define FALSEPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"

#include "../slottracker.h"

namespace borealis {

class FalsePredicate: public Predicate {

public:

	FalsePredicate(const llvm::Value* v, SlotTracker* st);
	virtual std::string toString() const;

private:

	const llvm::Value* v;
	const std::string vs;
	const std::string asString;

};

} /* namespace borealis */

#endif /* FALSEPREDICATE_H_ */

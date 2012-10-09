/*
 * BooleanPredicate.h
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#ifndef BOOLEANPREDICATE_H_
#define BOOLEANPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"

#include "../slottracker.h"

namespace borealis {

class BooleanPredicate: public Predicate {

public:

	BooleanPredicate(const llvm::Value* v, const bool b, SlotTracker* st);
	virtual std::string toString() const;
	virtual Predicate::Key getKey() const;
	virtual z3::expr toZ3(z3::context& ctx) const;

private:

	const llvm::Value* v;
	const bool b;
	const std::string vs;
	const std::string bs;
	const std::string asString;

};

} /* namespace borealis */

#endif /* BOOLEANPREDICATE_H_ */

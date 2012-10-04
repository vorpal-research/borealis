/*
 * TruePredicate.h
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#ifndef TRUEPREDICATE_H_
#define TRUEPREDICATE_H_

#include "Predicate.h"

#include "llvm/Value.h"

#include "../slottracker.h"

namespace borealis {

class TruePredicate: public Predicate {

public:

	TruePredicate(const llvm::Value* v, SlotTracker* st);
	virtual std::string toString() const;
	virtual Predicate::Key getKey() const;

private:

	const llvm::Value* v;
	const std::string vs;
	const std::string asString;

};

} /* namespace borealis */

#endif /* TRUEPREDICATE_H_ */

/*
 * StorePredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef STOREPREDICATE_H_
#define STOREPREDICATE_H_

#include "Predicate.h"

#include "llvm/Value.h"

#include "../slottracker.h"

namespace borealis {

class StorePredicate: public Predicate {

public:

	StorePredicate(
			const llvm::Value* lhv,
			const llvm::Value* rhv,
			SlotTracker* st);
	virtual std::string toString() const;

private:

	const std::string lhvs;
	const std::string rhvs;

	const llvm::Value* lhv;
	const llvm::Value* rhv;

	const std::string asString;

};

} /* namespace borealis */

#endif /* STOREPREDICATE_H_ */

/*
 * LoadPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef LOADPREDICATE_H_
#define LOADPREDICATE_H_


#include <llvm/Value.h>

#include "Predicate.h"

#include "../slottracker.h"

namespace borealis {

class LoadPredicate: public Predicate {

public:

	LoadPredicate(
			const llvm::Value* lhv,
			const llvm::Value* rhv,
			SlotTracker* st);
	virtual std::string toString() const;
	virtual Predicate::Key getKey() const;
	virtual z3::expr toZ3(z3::context& ctx) const;

private:

	const std::string lhvs;
	const std::string rhvs;

	const llvm::Value* lhv;
	const llvm::Value* rhv;

	const std::string asString;

};

} /* namespace borealis */

#endif /* LOADPREDICATE_H_ */

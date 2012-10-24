/*
 * StorePredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef STOREPREDICATE_H_
#define STOREPREDICATE_H_


#include <llvm/Value.h>

#include "Predicate.h"
#include "slottracker.h"

namespace borealis {

class StorePredicate: public Predicate {

public:

	StorePredicate(
			const llvm::Value* lhv,
			const llvm::Value* rhv,
			SlotTracker* st);
	virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

	virtual z3::expr toZ3(z3::context& ctx) const;

private:

	const std::string lhvs;
	const std::string rhvs;

	const llvm::Value* lhv;
	const llvm::Value* rhv;

};

} /* namespace borealis */

#endif /* STOREPREDICATE_H_ */

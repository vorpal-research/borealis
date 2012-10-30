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
#include "slottracker.h"

namespace borealis {

class BooleanPredicate: public Predicate {

public:

	BooleanPredicate(
			const llvm::Value* v,
			const bool b,
			SlotTracker* st);
	BooleanPredicate(
			const PredicateType type,
			const llvm::Value* v,
			const bool b,
			SlotTracker* st);
	virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

	virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

private:

	const llvm::Value* v;
	const bool b;
	const std::string _v;
	const std::string _b;

};

} /* namespace borealis */

#endif /* BOOLEANPREDICATE_H_ */

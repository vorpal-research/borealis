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
#include "Term/Term.h"

namespace borealis {

class BooleanPredicate: public Predicate {

public:

	BooleanPredicate(
			Term::Ptr v,
			bool b,
			SlotTracker* st);
	BooleanPredicate(
			PredicateType type,
			Term::Ptr v,
			bool b,
			SlotTracker* st);
	virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

	virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

private:

	const Term::Ptr v;
	const Term::Ptr b;

};

} /* namespace borealis */

#endif /* BOOLEANPREDICATE_H_ */

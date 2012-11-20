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

class PredicateFactory;

class BooleanPredicate: public Predicate {

public:

	virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

	virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

	friend class PredicateFactory;

private:

	const Term::Ptr v;
	const Term::Ptr b;

    BooleanPredicate(
            Term::Ptr v,
            bool b,
            SlotTracker* st);
    BooleanPredicate(
            PredicateType type,
            Term::Ptr v,
            bool b,
            SlotTracker* st);

};

} /* namespace borealis */

#endif /* BOOLEANPREDICATE_H_ */

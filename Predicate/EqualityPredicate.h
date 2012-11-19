/*
 * EqualityPredicate.h
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#ifndef EQUALITYPREDICATE_H_
#define EQUALITYPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"

namespace borealis {

class EqualityPredicate: public Predicate {

public:

    EqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            SlotTracker* st);
    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

private:

    const Term::Ptr lhv;
    const Term::Ptr rhv;

};

} /* namespace borealis */

#endif /* EQUALITYPREDICATE_H_ */

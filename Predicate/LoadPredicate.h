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

namespace borealis {

class LoadPredicate: public Predicate {

public:

    LoadPredicate(
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

#endif /* LOADPREDICATE_H_ */

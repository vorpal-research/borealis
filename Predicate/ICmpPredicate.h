/*
 * ICmpPredicate.hDEREF_VALUE
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef ICMPPREDICATE_H_
#define ICMPPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"

namespace borealis {

class PredicateFactory;

class ICmpPredicate: public Predicate {

public:

    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr op1;
    const Term::Ptr op2;

    const int cond;
    const std::string _cond;

    ICmpPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            int cond,
            SlotTracker* st);

};

} /* namespace borealis */

#endif /* ICMPPREDICATE_H_ */

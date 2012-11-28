#ifndef ALLOCAPREDICATE_H_
#define ALLOCAPREDICATE_H_

/*
 * AllocaPredicate.h
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#include "Predicate.h"

namespace borealis {

class AllocaPredicate: public Predicate {

    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef, Z3Context* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<AllocaPredicate>();
    }

    static bool classof(const AllocaPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const AllocaPredicate* accept(Transformer<SubClass>* t) const {
        return new AllocaPredicate(
                Term::Ptr(t->transform(lhv.get())),
                Term::Ptr(t->transform(numElements.get())));
    }

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr numElements;

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements);

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            SlotTracker* st);

};

} /* namespace borealis */

#endif /* ALLOCAPREDICATE_H_ */

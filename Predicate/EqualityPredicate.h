/*
 * EqualityPredicate.h
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#ifndef EQUALITYPREDICATE_H_
#define EQUALITYPREDICATE_H_

#include "Predicate.h"

namespace borealis {

class PredicateFactory;

class EqualityPredicate: public Predicate {

public:

    virtual Predicate::Key getKey() const;

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<EqualityPredicate>();
    }

    static bool classof(const EqualityPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const EqualityPredicate* accept(Transformer<SubClass>* t) const {
        return new EqualityPredicate(
                Term::Ptr(t->transform(lhv.get())),
                Term::Ptr(t->transform(rhv.get())));
    }

    virtual bool equals(const Predicate* other) const;
    virtual size_t hashCode() const;

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr rhv;

    EqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv);

    EqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            SlotTracker* st);

};

} /* namespace borealis */

#endif /* EQUALITYPREDICATE_H_ */

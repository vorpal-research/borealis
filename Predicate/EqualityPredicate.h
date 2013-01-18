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

class EqualityPredicate: public Predicate {

public:

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
                this->type,
                t->transform(lhv),
                t->transform(rhv));
    }

    virtual bool equals(const Predicate* other) const;
    virtual size_t hashCode() const;

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr rhv;

    EqualityPredicate(
            PredicateType type,
            Term::Ptr lhv,
            Term::Ptr rhv);
    EqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            SlotTracker* st,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* EQUALITYPREDICATE_H_ */

/*
 * InequalityPredicate.h
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#ifndef INEQUALITYPREDICATE_H_
#define INEQUALITYPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class InequalityPredicate: public borealis::Predicate {

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<InequalityPredicate>();
    }

    static bool classof(const InequalityPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const InequalityPredicate* accept(Transformer<SubClass>* t) const {
        return new InequalityPredicate(
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

    InequalityPredicate(
            PredicateType type,
            Term::Ptr lhv,
            Term::Ptr rhv);
    InequalityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            SlotTracker* st,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* INEQUALITYPREDICATE_H_ */

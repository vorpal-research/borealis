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

    typedef InequalityPredicate Self;

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const override;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<Self>();
    }

    static bool classof(const Self* /* p */) {
        return true;
    }

    template<class SubClass>
    const Self* accept(Transformer<SubClass>* t) const {
        return new Self(
            t->transform(lhv),
            t->transform(rhv),
            this->type
        );
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

    virtual Predicate* clone() const override {
        return new Self{ *this };
    }

    friend class PredicateFactory;

private:

    Term::Ptr lhv;
    Term::Ptr rhv;

    InequalityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE);
    InequalityPredicate(const Self&) = default;

};

} /* namespace borealis */

#endif /* INEQUALITYPREDICATE_H_ */

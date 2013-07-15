/*
 * StorePredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef STOREPREDICATE_H_
#define STOREPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class StorePredicate: public borealis::Predicate {

    typedef StorePredicate Self;

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const override;

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

    StorePredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE);
    StorePredicate(const Self&) = default;

};

} /* namespace borealis */

#endif /* STOREPREDICATE_H_ */

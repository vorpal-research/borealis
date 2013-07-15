/*
 * MallocPredicate.h
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#ifndef MALLOCPREDICATE_H_
#define MALLOCPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class MallocPredicate: public borealis::Predicate {

    typedef MallocPredicate Self;

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
            t->transform(numElements),
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
    Term::Ptr numElements;

    MallocPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            PredicateType type = PredicateType::STATE);
    MallocPredicate(const Self&) = default;

};

} /* namespace borealis */

#endif /* MALLOCPREDICATE_H_ */

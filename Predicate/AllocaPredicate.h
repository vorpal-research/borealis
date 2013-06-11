/*
 * AllocaPredicate.h
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#ifndef ALLOCAPREDICATE_H_
#define ALLOCAPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class AllocaPredicate: public borealis::Predicate {

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const override;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<AllocaPredicate>();
    }

    static bool classof(const AllocaPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const AllocaPredicate* accept(Transformer<SubClass>* t) const {
        return new AllocaPredicate(
                t->transform(lhv),
                t->transform(numElements),
                this->type);
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

    friend class PredicateFactory;

private:

    Term::Ptr lhv;
    Term::Ptr numElements;

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* ALLOCAPREDICATE_H_ */

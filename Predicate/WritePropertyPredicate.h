/*
 * WritePropertyPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef WRITEPROPERTYPREDICATE_H_
#define WRITEPROPERTYPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class WritePropertyPredicate: public borealis::Predicate {

    typedef WritePropertyPredicate self;

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const override;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<self>();
    }

    static bool classof(const self* /* p */) {
        return true;
    }

    template<class SubClass>
    const self* accept(Transformer<SubClass>* t) const {
        return new self(
                t->transform(propName),
                t->transform(lhv),
                t->transform(rhv),
                this->type);
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

    friend class PredicateFactory;

private:

    Term::Ptr propName;
    Term::Ptr lhv;
    Term::Ptr rhv;

    WritePropertyPredicate(
            Term::Ptr propName,
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* WRITEPROPERTYPREDICATE_H_ */
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

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<MallocPredicate>();
    }

    static bool classof(const MallocPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const MallocPredicate* accept(Transformer<SubClass>* t) const {
        return new MallocPredicate(
                t->transform(lhv),
                t->transform(numElements),
                this->type);
    }

    virtual bool equals(const Predicate* other) const;
    virtual size_t hashCode() const;

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr numElements;

    MallocPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* MALLOCPREDICATE_H_ */

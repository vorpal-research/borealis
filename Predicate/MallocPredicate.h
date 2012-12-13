/*
 * MallocPredicate.h
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#ifndef MALLOCPREDICATE_H_
#define MALLOCPREDICATE_H_

#include "Predicate.h"

namespace borealis {

class MallocPredicate: public Predicate {

public:

    virtual Predicate::Key getKey() const;

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
                Term::Ptr(t->transform(lhv.get()))
        );
    }

    virtual bool equals(const Predicate* other) const;
    virtual size_t hashCode() const;

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;

    MallocPredicate(Term::Ptr lhv);
    MallocPredicate(Term::Ptr lhv, SlotTracker* st);

};

} /* namespace borealis */

#endif /* MALLOCPREDICATE_H_ */

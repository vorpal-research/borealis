/*
 * LoadPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef LOADPREDICATE_H_
#define LOADPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class LoadPredicate: public borealis::Predicate {

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<LoadPredicate>();
    }

    static bool classof(const LoadPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const LoadPredicate* accept(Transformer<SubClass>* t) const {
        return new LoadPredicate(
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

    LoadPredicate(
            PredicateType type,
            Term::Ptr lhv,
            Term::Ptr rhv);
    LoadPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            SlotTracker* st,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* LOADPREDICATE_H_ */

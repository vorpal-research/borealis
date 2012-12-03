/*
 * AllocaPredicate.h
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#ifndef ALLOCAPREDICATE_H_
#define ALLOCAPREDICATE_H_

#include "Predicate.h"

namespace borealis {

class PredicateFactory;

class AllocaPredicate: public Predicate {

public:

    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef, Z3Context* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<AllocaPredicate>();
    }

    static bool classof(const AllocaPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const AllocaPredicate* accept(Transformer<SubClass>* t) const {
        return new AllocaPredicate(
                Term::Ptr(t->transform(lhv.get())),
                Term::Ptr(t->transform(numElements.get())));
    }

    virtual bool equals(const Predicate* other) const {
        if (other == nullptr) return false;
        if (this == other) return true;
        if (const AllocaPredicate* o = llvm::dyn_cast<AllocaPredicate>(other)) {
            return this->lhv == o->lhv &&
                    this->numElements == o->numElements;
        } else {
            return false;
        }
    }

    virtual size_t hashCode() const {
        size_t hash = 3;
        hash = 17 * hash + lhv->hashCode();
        hash = 17 * hash + numElements->hashCode();
        return hash;
    }

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr numElements;

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements);

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            SlotTracker* st);

};

} /* namespace borealis */

#endif /* ALLOCAPREDICATE_H_ */

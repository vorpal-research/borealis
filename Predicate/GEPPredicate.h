/*
 * GEPPredicate.h
 *
 *  Created on: Oct 23, 2012
 *      Author: ice-phoenix
 */

#ifndef GEPPREDICATE_H_
#define GEPPREDICATE_H_

#include <algorithm>
#include <tuple>

#include "Predicate.h"

namespace borealis {

class PredicateFactory;

class GEPPredicate: public Predicate {

public:

    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<GEPPredicate>();
    }

    static bool classof(const GEPPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const GEPPredicate* accept(Transformer<SubClass>* t) const {

        std::vector< std::pair< Term::Ptr, Term::Ptr > > new_shifts(shifts.size());
        std::transform(shifts.begin(), shifts.end(), new_shifts.begin(),
        [t](const std::pair< Term::Ptr, Term::Ptr >& e) {
            return std::make_pair(
                    Term::Ptr(t->transform(e.first.get())),
                    Term::Ptr(t->transform(e.second.get())));
        });

        return new GEPPredicate(
                Term::Ptr(t->transform(lhv.get())),
                Term::Ptr(t->transform(rhv.get())),
                new_shifts);
    }

    virtual bool equals(const Predicate* other) const {
        if (other == nullptr) return false;
        if (this == other) return true;
        if (const GEPPredicate* o = llvm::dyn_cast<GEPPredicate>(other)) {
            return *this->lhv == *o->lhv &&
                    *this->rhv == *o->rhv;
            // FIXME: akhin Compare this->shifts and other->shifts
        } else {
            return false;
        }
    }

    virtual size_t hashCode() const {
        size_t hash = 3;
        hash = 17 * hash + lhv->hashCode();
        hash = 17 * hash + rhv->hashCode();
        // FIXME: akhin Hash this->shifts as well
        return hash;
    }

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr rhv;
    std::vector< std::pair< Term::Ptr, Term::Ptr > > shifts;

    GEPPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            std::vector< std::pair< Term::Ptr, Term::Ptr > >& shifts);

    GEPPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            std::vector< std::pair<llvm::Value*, uint64_t> > shifts,
            SlotTracker* st);

};

} /* namespace borealis */

#endif /* GEPPREDICATE_H_ */

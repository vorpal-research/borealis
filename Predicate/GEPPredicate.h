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

#include "Predicate/Predicate.h"

namespace borealis {

class GEPPredicate: public borealis::Predicate {

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<GEPPredicate>();
    }

    static bool classof(const GEPPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const GEPPredicate* accept(Transformer<SubClass>* t) const {

        std::vector< std::pair< Term::Ptr, Term::Ptr > > new_shifts;
        new_shifts.reserve(shifts.size());
        std::transform(shifts.begin(), shifts.end(), new_shifts.begin(),
            [t](const std::pair< Term::Ptr, Term::Ptr >& e) {
                return std::make_pair(t->transform(e.first), t->transform(e.second));
            }
        );

        return new GEPPredicate(
                this->type,
                t->transform(lhv),
                t->transform(rhv),
                new_shifts);
    }

    virtual bool equals(const Predicate* other) const;
    virtual size_t hashCode() const;

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr rhv;
    std::vector< std::pair< Term::Ptr, Term::Ptr > > shifts;

    GEPPredicate(
            PredicateType type,
            Term::Ptr lhv,
            Term::Ptr rhv,
            std::vector< std::pair< Term::Ptr, Term::Ptr > >& shifts);
    GEPPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            std::vector< std::pair< Term::Ptr, Term::Ptr > >& shifts,
            SlotTracker* st,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* GEPPREDICATE_H_ */

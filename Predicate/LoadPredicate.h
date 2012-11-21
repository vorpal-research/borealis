/*
 * LoadPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef LOADPREDICATE_H_
#define LOADPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"

namespace borealis {

class PredicateFactory;

class LoadPredicate: public Predicate {

public:

    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<LoadPredicate>();
    }

    static bool classof(const LoadPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const LoadPredicate* accept(Transformer<SubClass>* t) {
        return new LoadPredicate(
                t->transform(lhv),
                t->transform(rhv));
    }

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr rhv;

    LoadPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv);

    LoadPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            SlotTracker* st);

};

} /* namespace borealis */

#endif /* LOADPREDICATE_H_ */

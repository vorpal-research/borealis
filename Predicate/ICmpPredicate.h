/*
 * ICmpPredicate.hDEREF_VALUE
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef ICMPPREDICATE_H_
#define ICMPPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"

namespace borealis {

class PredicateFactory;

class ICmpPredicate: public Predicate {

public:

    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef, Z3Context* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<ICmpPredicate>();
    }

    static bool classof(const ICmpPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const ICmpPredicate* accept(Transformer<SubClass>* t) const {
        return new ICmpPredicate(
                Term::Ptr(t->transform(lhv.get())),
                Term::Ptr(t->transform(op1.get())),
                Term::Ptr(t->transform(op2.get())),
                cond);
    }

    virtual bool equals(const Predicate* other) const {
        if (other == nullptr) return false;
        if (this == other) return true;
        if (const ICmpPredicate* o = llvm::dyn_cast<ICmpPredicate>(other)) {
            return this->lhv == o->lhv &&
                    this->op1 == o->op1 &&
                    this->op2 == o->op2 &&
                    this->cond == o->cond;
        } else {
            return false;
        }
    }

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr op1;
    const Term::Ptr op2;

    const int cond;
    const std::string _cond;

    ICmpPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            int cond);

    ICmpPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            int cond,
            SlotTracker* st);

};

} /* namespace borealis */

#endif /* ICMPPREDICATE_H_ */

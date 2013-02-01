/*
 * ICmpPredicate.hDEREF_VALUE
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef ICMPPREDICATE_H_
#define ICMPPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class ICmpPredicate: public borealis::Predicate {

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<ICmpPredicate>();
    }

    static bool classof(const ICmpPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const ICmpPredicate* accept(Transformer<SubClass>* t) const {
        return new ICmpPredicate(
                this->type,
                t->transform(lhv),
                t->transform(op1),
                t->transform(op2),
                cond);
    }

    virtual bool equals(const Predicate* other) const;
    virtual size_t hashCode() const;

    friend class PredicateFactory;

private:

    const Term::Ptr lhv;
    const Term::Ptr op1;
    const Term::Ptr op2;

    const llvm::ConditionType cond;
    const std::string _cond;

    ICmpPredicate(
            PredicateType type,
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            llvm::ConditionType cond);
    ICmpPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            llvm::ConditionType cond,
            SlotTracker* st,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* ICMPPREDICATE_H_ */

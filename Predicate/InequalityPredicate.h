/*
 * InequalityPredicate.h
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#ifndef INEQUALITYPREDICATE_H_
#define INEQUALITYPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/InequalityPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message InequalityPredicate {
    extend borealis.proto.Predicate {
        optional InequalityPredicate ext = $COUNTER_PRED;
    }

    optional Term lhv = 1;
    optional Term rhv = 2;
}

**/
class InequalityPredicate: public borealis::Predicate {

    InequalityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(InequalityPredicate);

    Term::Ptr getLhv() const;
    Term::Ptr getRhv() const;

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto&& _lhv = t->transform(getLhv());
        auto&& _rhv = t->transform(getRhv());
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            getLhv() != _lhv || getRhv() != _rhv,
            new Self( _lhv, _rhv, _loc, _type )
        );
    }

};

template<class Impl>
struct SMTImpl<Impl, InequalityPredicate> {
    static Bool<Impl> doit(
            const InequalityPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;
        return SMT<Impl>::doit(p->getLhv(), ef, ctx) != SMT<Impl>::doit(p->getRhv(), ef, ctx);
    }
};

} /* namespace borealis */

#endif /* INEQUALITYPREDICATE_H_ */

/*
 * EqualityPredicate.h
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#ifndef EQUALITYPREDICATE_H_
#define EQUALITYPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/EqualityPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message EqualityPredicate {
    extend borealis.proto.Predicate {
        optional EqualityPredicate ext = $COUNTER_PRED;
    }

    optional Term lhv = 1;
    optional Term rhv = 2;
}

**/
class EqualityPredicate: public borealis::Predicate {

    EqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(EqualityPredicate);

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
struct SMTImpl<Impl, EqualityPredicate> {
    static Bool<Impl> doit(
            const EqualityPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;
        return SMT<Impl>::doit(p->getLhv(), ef, ctx) == SMT<Impl>::doit(p->getRhv(), ef, ctx);
    }
};

struct EqualityPredicateExtractor {

    functional_hell::matchers::storage_t<Term::Ptr, Term::Ptr> unapply(Predicate::Ptr pred) const {
        if (auto&& p = llvm::dyn_cast<EqualityPredicate>(pred)) {
            return functional_hell::matchers::make_storage(p->getLhv(), p->getRhv());
        } else {
            return {};
        }
    }

};

static auto $EqualityPredicate = functional_hell::matchers::make_pattern(EqualityPredicateExtractor());

} /* namespace borealis */

#endif /* EQUALITYPREDICATE_H_ */

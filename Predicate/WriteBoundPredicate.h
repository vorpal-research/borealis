/*
 * WriteBoundPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef WRITEBOUNDPREDICATE_H_
#define WRITEBOUNDPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/WriteBoundPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message WriteBoundPredicate {
    extend borealis.proto.Predicate {
        optional WriteBoundPredicate ext = $COUNTER_PRED;
    }

    optional Term lhv = 1;
    optional Term rhv = 2;
}

**/
class WriteBoundPredicate: public borealis::Predicate {

    WriteBoundPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(WriteBoundPredicate);

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

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, WriteBoundPredicate> {
    static Bool<Impl> doit(
            const WriteBoundPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        Pointer lp = SMT<Impl>::doit(p->getLhv(), ef, ctx);
        ASSERT(lp, "Bound write with a non-pointer value");

        Integer ri = SMT<Impl>::doit(p->getRhv(), ef, ctx);
        ASSERT(ri, "Bound write with a non-integer value");

        ctx->writeBound(lp, ri);

        return ef.getTrue();
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* WRITEBOUNDPREDICATE_H_ */

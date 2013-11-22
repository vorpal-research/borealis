/*
 * StorePredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef STOREPREDICATE_H_
#define STOREPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/StorePredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message StorePredicate {
    extend borealis.proto.Predicate {
        optional StorePredicate ext = $COUNTER_PRED;
    }

    optional Term lhv = 1;
    optional Term rhv = 2;
}

**/
class StorePredicate: public borealis::Predicate {

    Term::Ptr lhv;
    Term::Ptr rhv;

    StorePredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(StorePredicate);

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto _lhv = t->transform(lhv);
        auto _rhv = t->transform(rhv);
        auto _type = type;
        PREDICATE_ON_CHANGED(
            lhv != _lhv || rhv != _rhv,
            new Self( _lhv, _rhv, _type )
        );
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, StorePredicate> {
    static Bool<Impl> doit(
            const StorePredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        auto l = SMT<Impl>::doit(p->getLhv(), ef, ctx).template to<Pointer>();
        ASSERT(!l.empty(), "Store dealing with a non-pointer value");
        auto lp = l.getUnsafe();

        auto r = SMT<Impl>::doit(p->getRhv(), ef, ctx);

        ctx->writeExprToMemory(lp, r);

        return ef.getTrue();
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* STOREPREDICATE_H_ */

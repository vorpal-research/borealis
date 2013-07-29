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
    const Self* accept(Transformer<SubClass>* t) const {
        return new Self{
            t->transform(lhv),
            t->transform(rhv),
            type
        };
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

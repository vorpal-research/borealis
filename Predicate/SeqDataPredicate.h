/*
 * SeqDataPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef SEQDATAPREDICATE_H_
#define SEQDATAPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/SeqDataPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message SeqDataPredicate {
    extend borealis.proto.Predicate {
        optional SeqDataPredicate ext = 24;
    }

    optional Term base = 1;
    repeated Term data = 2;
}

**/
class SeqDataPredicate: public borealis::Predicate {

    Term::Ptr base;
    std::vector<Term::Ptr> data;

    SeqDataPredicate(
            Term::Ptr base,
            const std::vector<Term::Ptr>& data,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(SeqDataPredicate);

    Term::Ptr getBase() const { return base; }
    const std::vector<Term::Ptr>& getData() const { return data; }

    template<class SubClass>
    const Self* accept(Transformer<SubClass>* t) const {
        return new Self{
            t->transform(base),
            util::viewContainer(data)
                .map([&t](const Term::Ptr& d) { return t->transform(d); })
                .toVector(),
            type
        };
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, SeqDataPredicate> {
    static Bool<Impl> doit(
            const SeqDataPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        auto l = SMT<Impl>::doit(p->getBase(), ef, ctx).template to<Pointer>();
        ASSERT(!l.empty(), "SeqData with non-pointer base");
        auto lp = l.getUnsafe();

        auto base = ctx->getGlobalPtr(p->getData().size());
        ctx->writeExprToMemory(lp, base);

        for (const auto& datum : p->getData()) {
            auto d = SMT<Impl>::doit(datum, ef, ctx);
            ctx->writeExprToMemory(base, d);
            base = base + 1;
        }

        return ef.getTrue();
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* SEQDATAPREDICATE_H_ */

/*
 * SeqDataPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef SEQDATAPREDICATE_H_
#define SEQDATAPREDICATE_H_

#include "Config/config.h"
#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/SeqDataPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message SeqDataPredicate {
    extend borealis.proto.Predicate {
        optional SeqDataPredicate ext = $COUNTER_PRED;
    }

    optional Term base = 1;
    repeated Term data = 2;
}

**/
class SeqDataPredicate: public borealis::Predicate {

    SeqDataPredicate(
            Term::Ptr base,
            const std::vector<Term::Ptr>& data,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(SeqDataPredicate);

    Term::Ptr getBase() const;
    auto getData() const -> decltype(util::viewContainer(ops));

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto&& _base = t->transform(getBase());
        auto&& _data = getData().map(
            [&](auto&& d) { return t->transform(d); }
        );
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            getBase() != _base || not util::equal(getData(), _data, util::equality()),
            new Self( _base, _data.toVector(), _loc, _type )
        );
    }

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

        auto&& l = SMT<Impl>::doit(p->getBase(), ef, ctx).template to<Pointer>();
        ASSERT(not l.empty(), "SeqData with non-pointer base");
        auto&& lp = l.getUnsafe();

        auto&& base = ctx->getGlobalPtr(p->getData().size());
        auto&& res = lp == base;

        static config::ConfigEntry<bool> SkipStaticInit("analysis", "skip-static-init");
        if (not SkipStaticInit.get(true)) {
            for (auto&& datum : p->getData()) {
                auto&& d = SMT<Impl>::doit(datum, ef, ctx);
                ctx->writeExprToMemory(base, d);
                base = base + 1;
            }
        }

        return res;
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* SEQDATAPREDICATE_H_ */

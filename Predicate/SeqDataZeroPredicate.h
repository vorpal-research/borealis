/*
 * SeqDataZeroPredicate.h
 *
 *  Created on: May 1, 2015
 *      Author: belyaev
 */

#ifndef SEQDATAZEROPREDICATE_H_
#define SEQDATAZEROPREDICATE_H_

#include "Config/config.h"
#include "Predicate/Predicate.h"
#include "Logging/tracer.hpp"

namespace borealis {

/** protobuf -> Predicate/SeqDataZeroPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message SeqDataZeroPredicate {
    extend borealis.proto.Predicate {
        optional SeqDataZeroPredicate ext = $COUNTER_PRED;
    }

    optional Term base = 1;
    optional uint32 size = 2;
}

**/
class SeqDataZeroPredicate: public borealis::Predicate {

    SeqDataZeroPredicate(
            Term::Ptr base,
            size_t size,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

    size_t size;

public:

    MK_COMMON_PREDICATE_IMPL(SeqDataZeroPredicate);

    Term::Ptr getBase() const;
    size_t getSize() const{ return size; }

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        TRACE_FUNC;
        auto&& _base = t->transform(getBase());
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            getBase() != _base,
            new Self( _base, getSize(), _loc, _type )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, SeqDataZeroPredicate> {
    static Bool<Impl> doit(
            const SeqDataZeroPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        Pointer lp = SMT<Impl>::doit(p->getBase(), ef, ctx);
        ASSERT(lp, "SeqData with non-pointer base");

        auto&& base = ctx->getGlobalPtr(p->getSize());
        auto&& res = lp == base;
        auto&& zero = ef.getIntConst(0);

        static config::ConfigEntry<bool> SkipStaticInit("analysis", "skip-static-init");
        static config::ConfigEntry<bool> UseRangedStore("analysis", "use-range-stores");
        if (not SkipStaticInit.get(true)) {
            if (UseRangedStore.get(false)) {
                ctx->writeExprRangeToMemory(base, p->getSize(), zero);
            } else {
                for (auto i = 0U; i < p->getSize(); ++i) {
                    ctx->writeExprToMemory(base + i, zero);
                }
            }
        }

        return res;
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* SEQDATAZEROPREDICATE_H_ */

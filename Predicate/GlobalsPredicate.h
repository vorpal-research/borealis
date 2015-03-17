/*
 * GlobalsPredicate.h
 *
 *  Created on: Mar 13, 2013
 *      Author: ice-phoenix
 */

#ifndef GLOBALSPREDICATE_H_
#define GLOBALSPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/GlobalsPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message GlobalsPredicate {
    extend borealis.proto.Predicate {
        optional GlobalsPredicate ext = $COUNTER_PRED;
    }

    repeated Term globals = 1;
}

**/
class GlobalsPredicate: public borealis::Predicate {

    GlobalsPredicate(
            const std::vector<Term::Ptr>& globals,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(GlobalsPredicate);

    auto getGlobals() const -> decltype(util::viewContainer(ops));

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto&& _globals = getGlobals().map(
            [&](auto&& e) { return t->transform(e); }
        );
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            not util::equal(getGlobals(), _globals, ops::equals_to),
            new Self( _globals.toVector(), _loc, _type )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, GlobalsPredicate> {
    static Bool<Impl> doit(
            const GlobalsPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl)

        ASSERTC(ctx != nullptr);

        auto&& res = ef.getTrue();
        for (auto&& g : p->getGlobals()) {
            auto&& ge = SMT<Impl>::doit(g, ef, ctx).template to<Pointer>();
            ASSERT(not ge.empty(), "Encountered non-Pointer global value: " + g->getName());
            auto&& gp = ge.getUnsafe();
            res = res && gp == ctx->getGlobalPtr();
        }
        return res;
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* GLOBALSPREDICATE_H_ */

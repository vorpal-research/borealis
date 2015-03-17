/*
 * DefaultSwitchCasePredicate.h
 *
 *  Created on: Jan 17, 2013
 *      Author: ice-phoenix
 */

#ifndef DEFAULTSWITCHCASEPREDICATE_H_
#define DEFAULTSWITCHCASEPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/DefaultSwitchCasePredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message DefaultSwitchCasePredicate {
    extend borealis.proto.Predicate {
        optional DefaultSwitchCasePredicate ext = $COUNTER_PRED;
    }

    optional Term cond = 1;
    repeated Term cases = 2;
}

**/
class DefaultSwitchCasePredicate: public borealis::Predicate {

    DefaultSwitchCasePredicate(
            Term::Ptr cond,
            const std::vector<Term::Ptr>& cases,
            const Locus& loc,
            PredicateType type = PredicateType::PATH);

public:

    MK_COMMON_PREDICATE_IMPL(DefaultSwitchCasePredicate);

    Term::Ptr getCond() const;
    auto getCases() const -> decltype(util::viewContainer(ops));

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto&& _cond = t->transform(getCond());
        auto&& _cases = getCases().map(
            [&](auto&& e) { return t->transform(e); }
        );
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            getCond() != _cond || not util::equal(getCases(), _cases, ops::equals_to),
            new Self( _cond, _cases.toVector(), _loc, _type )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, DefaultSwitchCasePredicate> {
    static Bool<Impl> doit(
            const DefaultSwitchCasePredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl)

        auto&& le = SMT<Impl>::doit(p->getCond(), ef, ctx).template to<Integer>();
        ASSERT(not le.empty(), "Encountered switch with non-Integer condition");

        auto&& res = ef.getTrue();
        for (auto&& c : p->getCases()) {
            auto&& re = SMT<Impl>::doit(c, ef, ctx).template to<Integer>();
            ASSERT(not re.empty(), "Encountered switch with non-Integer case");
            res = res && le.getUnsafe() != re.getUnsafe();
        }
        return res;
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* DEFAULTSWITCHCASEPREDICATE_H_ */

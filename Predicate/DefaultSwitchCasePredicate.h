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

    Term::Ptr cond;
    const std::vector<Term::Ptr> cases;

    DefaultSwitchCasePredicate(
            Term::Ptr cond,
            const std::vector<Term::Ptr>& cases,
            PredicateType type = PredicateType::PATH);

public:

    MK_COMMON_PREDICATE_IMPL(DefaultSwitchCasePredicate);

    Term::Ptr getCond() const { return cond; }
    const std::vector<Term::Ptr> getCases() const { return cases; }

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto _cond = t->transform(cond);
        auto _cases = util::viewContainer(cases).map(
            [&t](const Term::Ptr& e) { return t->transform(e); }
        ).toVector();
        auto _type = type;
        PREDICATE_ON_CHANGED(
            cond != _cond || cases != _cases,
            new Self( _cond, _cases, _type )
        );
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

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

        auto le = SMT<Impl>::doit(p->getCond(), ef, ctx).template to<Integer>();
        ASSERT(!le.empty(), "Encountered switch with non-Integer condition");

        auto res = ef.getTrue();
        for (const auto& c : p->getCases()) {
            auto re = SMT<Impl>::doit(c, ef, ctx).template to<Integer>();
            ASSERT(!re.empty(), "Encountered switch with non-Integer case");
            res = res && le.getUnsafe() != re.getUnsafe();
        }
        return res;
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* DEFAULTSWITCHCASEPREDICATE_H_ */

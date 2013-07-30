/*
 * DefaultSwitchCasePredicate.h
 *
 *  Created on: Jan 17, 2013
 *      Author: ice-phoenix
 */

#ifndef DEFAULTSWITCHCASEPREDICATE_H_
#define DEFAULTSWITCHCASEPREDICATE_H_

#include "Protobuf/Gen/Predicate/DefaultSwitchCasePredicate.pb.h"

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/DefaultSwitchCasePredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message DefaultSwitchCasePredicate {
    extend borealis.proto.Predicate {
        optional DefaultSwitchCasePredicate ext = 17;
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
    const Self* accept(Transformer<SubClass>* t) const {
        std::vector<Term::Ptr> new_cases;
        new_cases.reserve(cases.size());
        std::transform(cases.begin(), cases.end(), std::back_inserter(new_cases),
            [&t](const Term::Ptr& e) { return t->transform(e); }
        );

        return new Self{
            t->transform(cond),
            new_cases,
            type
        };
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



template<class FN>
struct ConverterImpl<DefaultSwitchCasePredicate, proto::DefaultSwitchCasePredicate, FN> {

    typedef Converter<Term, proto::Term, FN> TermConverter;

    static proto::DefaultSwitchCasePredicate* toProtobuf(const DefaultSwitchCasePredicate* p) {
        auto res = util::uniq(new proto::DefaultSwitchCasePredicate());
        res->set_allocated_cond(
            TermConverter::toProtobuf(p->getCond()).release()
        );
        for (const auto& c : p->getCases()) {
            res->mutable_cases()->AddAllocated(
                TermConverter::toProtobuf(c).release()
            );
        }
        return res.release();
    }

    static Predicate::Ptr fromProtobuf(
            FN fn,
            PredicateType type,
            const proto::DefaultSwitchCasePredicate& p) {
        auto cond = TermConverter::fromProtobuf(fn, p.cond());

        std::vector<Term::Ptr> cases;
        cases.reserve(p.cases_size());
        for (const auto& c : p.cases()) {
            cases.push_back(
                TermConverter::fromProtobuf(fn, c)
            );
        }

        return Predicate::Ptr{ new DefaultSwitchCasePredicate(cond, cases, type) };
    }
};

} /* namespace borealis */

#endif /* DEFAULTSWITCHCASEPREDICATE_H_ */

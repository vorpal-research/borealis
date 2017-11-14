//
// Created by abdullin on 11/13/17.
//

#ifndef BOREALIS_CALLPREDICATE_H
#define BOREALIS_CALLPREDICATE_H

#include "Config/config.h"
#include "Predicate/Predicate.h"
#include "Logging/tracer.hpp"

namespace borealis {

/** protobuf -> Predicate/CallPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message CallPredicate {
    extend borealis.proto.Predicate {
        optional CallPredicate ext = $COUNTER_PRED;
    }

    optional Term lhv = 1;
    optional Term function = 2;
    repeated Term args = 3;
}

**/

class CallPredicate: public borealis::Predicate {

    bool hasLhv_;

    CallPredicate(
            Term::Ptr lhv,
            Term::Ptr function,
            const std::vector<Term::Ptr>& args,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(CallPredicate);

    bool hasLhv() const;
    Term::Ptr getLhv() const;
    Term::Ptr getFunctionName() const;
    auto getArgs() const -> decltype(util::viewContainer(ops));

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        TRACE_FUNC;
        Term::Ptr _lhv = nullptr;
        if(hasLhv()) {
            _lhv = t->transform(getLhv());
        }
        auto&& _funName = t->transform(getFunctionName());
        auto&& _args = getArgs().map(
                [&](auto&& d) { return t->transform(d); }
        );
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
                getFunctionName() != _funName || getLhv() != _lhv || not util::equal(getArgs(), _args, ops::equals_to),
                new Self(_lhv, _funName, _args.toVector(), _loc, _type )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, CallPredicate> {
    static Bool<Impl> doit(
            const CallPredicate*,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        return ef.getTrue();
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif //BOREALIS_CALLPREDICATE_H

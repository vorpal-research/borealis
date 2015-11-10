/*
 * ArgumentCountTerm.h
 *
 *  Created on: Oct 30, 2015
 *      Author: belyaev
 */

#ifndef ARGUMENTCOUNTTERM_H_
#define ARGUMENTCOUNTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/ArgumentCountTerm.proto
import "Term/Term.proto";

package borealis.proto;

message ArgumentCountTerm {
    extend borealis.proto.Term {
        optional ArgumentCountTerm ext = $COUNTER_TERM;
    }
}

**/
class ArgumentCountTerm: public borealis::Term {

    ArgumentCountTerm(Type::Ptr type);

public:

    MK_COMMON_TERM_IMPL(ArgumentCountTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, ArgumentCountTerm> {
    static Dynamic<Impl> doit(
            const ArgumentCountTerm*,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        // since it hasn't been materialized, it can be anything,
        // so let's make it a free var
        return ef.getIntVar("$$bor.argcount$$");
    }
};

} /* namespace borealis */

#endif /* ARGUMENTCOUNTTERM_H_ */

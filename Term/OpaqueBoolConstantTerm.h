/*
 * OpaqueBoolConstantTerm.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEBOOLCONSTANTTERM_H_
#define OPAQUEBOOLCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueBoolConstantTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueBoolConstantTerm {
    extend borealis.proto.Term {
        optional OpaqueBoolConstantTerm ext = $COUNTER_TERM;
    }

    optional bool value = 1;
}

**/
class OpaqueBoolConstantTerm: public borealis::Term {

    bool value;

    OpaqueBoolConstantTerm(Type::Ptr type, bool value);

public:

    MK_COMMON_TERM_IMPL(OpaqueBoolConstantTerm);

    bool getValue() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueBoolConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueBoolConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getBoolConst(t->getValue());
    }
};

struct OpaqueBoolConstantTermExtractor {

    auto unapply(Term::Ptr t) const -> functional_hell::matchers::storage_t<bool> {
        if (auto&& tt = llvm::dyn_cast<OpaqueBoolConstantTerm>(t)) {
            return functional_hell::matchers::make_storage(tt->getValue());
        } else {
            return {};
        }
    }

};

static auto $OpaqueBoolConstantTerm = functional_hell::matchers::make_pattern(OpaqueBoolConstantTermExtractor());


} /* namespace borealis */

#endif /* OPAQUEBOOLCONSTANTTERM_H_ */

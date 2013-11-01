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

    OpaqueBoolConstantTerm(Type::Ptr type, bool value):
        Term(
            class_tag(*this),
            type,
            value ? "true" : "false"
        ), value(value) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueBoolConstantTerm);

    bool getValue() const { return value; }

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
        return ef.getBoolConst(t->getValue());
    }
};

} /* namespace borealis */

#endif /* OPAQUEBOOLCONSTANTTERM_H_ */

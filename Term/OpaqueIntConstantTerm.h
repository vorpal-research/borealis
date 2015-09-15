/*
 * OpaqueConstantTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEINTCONSTANTTERM_H_
#define OPAQUEINTCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueIntConstantTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueIntConstantTerm {
    extend borealis.proto.Term {
        optional OpaqueIntConstantTerm ext = $COUNTER_TERM;
    }

    optional sint64 value = 1;
}

**/
class OpaqueIntConstantTerm: public borealis::Term {

    long long value;

    OpaqueIntConstantTerm(Type::Ptr type, long long value);

public:

    MK_COMMON_TERM_IMPL(OpaqueIntConstantTerm);

    long long getValue() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueIntConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueIntConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getIntConst(t->getValue(), ExprFactory<Impl>::sizeForType(t->getType()));
    }
};

} /* namespace borealis */

#endif /* OPAQUEINTCONSTANTTERM_H_ */

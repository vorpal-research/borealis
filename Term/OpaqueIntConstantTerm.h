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

template<class T> struct AllocationPoint;

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
    int64_t value;

    OpaqueIntConstantTerm(Type::Ptr type, int64_t value);

public:

    MK_COMMON_TERM_IMPL(OpaqueIntConstantTerm);

    int64_t getValue() const;

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
        auto&& size = ExprFactory<Impl>::sizeForType(t->getType());
        if(size > sizeof(int) * 8) {
            return ef.getIntConst(util::toString(t->getValue()), size);
        }
        return ef.getIntConst(t->getValue(), size);
    }
};

} /* namespace borealis */

#endif /* OPAQUEINTCONSTANTTERM_H_ */

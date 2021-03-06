/*
 * OpaqueNullPtrTerm.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef OPAQUENULLPTRTERM_H_
#define OPAQUENULLPTRTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueNullPtrTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueNullPtrTerm {
    extend borealis.proto.Term {
        optional OpaqueNullPtrTerm ext = $COUNTER_TERM;
    }
}

**/
class OpaqueNullPtrTerm: public borealis::Term {

    OpaqueNullPtrTerm(Type::Ptr type);

public:

    MK_COMMON_TERM_IMPL(OpaqueNullPtrTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueNullPtrTerm> {
    static Dynamic<Impl> doit(
            const OpaqueNullPtrTerm*,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getNullPtr();
    }
};

} /* namespace borealis */

#endif /* OPAQUENULLPTRTERM_H_ */

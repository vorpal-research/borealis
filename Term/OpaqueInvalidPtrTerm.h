/*
 * OpaqueInvalidPtrTerm.h
 *
 *  Created on: Sep 17, 2013
 *      Author: sam
 */

#ifndef OPAQUEINVALIDPTRTERM_H_
#define OPAQUEINVALIDPTRTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueInvalidPtrTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueInvalidPtrTerm {
    extend borealis.proto.Term {
        optional OpaqueInvalidPtrTerm ext = $COUNTER_TERM;
    }
}

**/
class OpaqueInvalidPtrTerm: public borealis::Term {

    OpaqueInvalidPtrTerm(Type::Ptr type);

public:

    MK_COMMON_TERM_IMPL(OpaqueInvalidPtrTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueInvalidPtrTerm> {
    static Dynamic<Impl> doit(
            const OpaqueInvalidPtrTerm*,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getInvalidPtr();
    }
};

} /* namespace borealis */

#endif /* OPAQUEINVALIDPTRTERM_H_ */

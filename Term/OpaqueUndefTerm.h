/*
 * OpaqueUndefTerm.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEUNDEFTERM_H_
#define OPAQUEUNDEFTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueUndefTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueUndefTerm {
    extend borealis.proto.Term {
        optional OpaqueUndefTerm ext = $COUNTER_TERM;
    }
}

**/
class OpaqueUndefTerm: public borealis::Term {

    OpaqueUndefTerm(Type::Ptr type);

public:

    MK_COMMON_TERM_IMPL(OpaqueUndefTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueUndefTerm> {
    static Dynamic<Impl> doit(
            const OpaqueUndefTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getVarByTypeAndName(t->getType(), t->getName(), /*fresh = */true);
    }
};


} /* namespace borealis */

#endif /* OPAQUEUNDEFTERM_H_ */

/*
 * OpaqueUndefTerm.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEUNDEFTERM_H_
#define OPAQUEUNDEFTERM_H_

#include "Protobuf/Gen/Term/OpaqueUndefTerm.pb.h"

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueUndefTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueUndefTerm {
    extend borealis.proto.Term {
        optional OpaqueUndefTerm ext = 27;
    }
}

**/
class OpaqueUndefTerm: public borealis::Term {

    OpaqueUndefTerm(Type::Ptr type):
        Term(
            class_tag(*this),
            type,
            "<undef>"
        ) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueUndefTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueUndefTerm> {
    static Dynamic<Impl> doit(
            const OpaqueUndefTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getVarByTypeAndName(t->getType(), t->getName(), true);
    }
};


} /* namespace borealis */

#endif /* OPAQUEUNDEFTERM_H_ */

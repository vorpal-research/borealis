/*
 * OpaqueConstantTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEINTCONSTANTTERM_H_
#define OPAQUEINTCONSTANTTERM_H_

#include "Protobuf/Gen/Term/OpaqueIntConstantTerm.pb.h"

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueIntConstantTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueIntConstantTerm {
    extend borealis.proto.Term {
        optional OpaqueIntConstantTerm ext = 25;
    }

    optional sint64 value = 1;
}

**/
class OpaqueIntConstantTerm: public borealis::Term {

    long long value;

    OpaqueIntConstantTerm(Type::Ptr type, long long value):
        Term(
            class_tag(*this),
            type,
            util::toString(value)
        ), value(value) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueIntConstantTerm);

    long long getValue() const { return value; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueIntConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueIntConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getIntConst(t->getValue());
    }
};

} /* namespace borealis */

#endif /* OPAQUEINTCONSTANTTERM_H_ */

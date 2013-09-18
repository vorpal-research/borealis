/*
 * InvalidPtrTerm.h
 *
 *  Created on: Sep 17, 2013
 *      Author: sam
 */

#ifndef INVALIDPTRTERM_H_
#define INVALIDPTRTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/InvalidPtrTerm.proto
import "Term/Term.proto";

package borealis.proto;

message InvalidPtrTerm {
    extend borealis.proto.Term {
        optional InvalidPtrTerm ext = 36;
    }
}

**/
class InvalidPtrTerm: public borealis::Term {

    InvalidPtrTerm(Type::Ptr type):
        Term(
            class_tag(*this),
            type,
            "<invalid>"
        ) {};

public:

    MK_COMMON_TERM_IMPL(InvalidPtrTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

template<class Impl>
struct SMTImpl<Impl, InvalidPtrTerm> {
    static Dynamic<Impl> doit(
            const InvalidPtrTerm*,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getInvalidPtr();
    }
};

} /* namespace borealis */

#endif /* INVALIDPTRTERM_H_ */

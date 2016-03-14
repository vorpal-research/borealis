/*
 * OpaqueConstantTerm.h
 *
 *  Created on: Mar 10, 2016
 *      Author: belyaev
 */

#ifndef OPAQUEBIGINTCONSTANTTERM_H_
#define OPAQUEBIGINTCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueBigIntConstantTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueBigIntConstantTerm {
    extend borealis.proto.Term {
        optional OpaqueBigIntConstantTerm ext = $COUNTER_TERM;
    }

    optional string representation = 1;
}

**/
class OpaqueBigIntConstantTerm: public borealis::Term {
    OpaqueBigIntConstantTerm(Type::Ptr type, int64_t value);
    OpaqueBigIntConstantTerm(Type::Ptr type, const std::string& representation);

public:

    MK_COMMON_TERM_IMPL(OpaqueBigIntConstantTerm);

    const std::string& getRepresentation() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueBigIntConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueBigIntConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getIntConst(t->getRepresentation(), ExprFactory<Impl>::sizeForType(t->getType()));
    }
};

} /* namespace borealis */

#endif /* OPAQUEBIGINTCONSTANTTERM_H_ */

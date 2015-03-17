/*
 * ReturnValueTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef RETURNVALUETERM_H_
#define RETURNVALUETERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/ReturnValueTerm.proto
import "Term/Term.proto";

package borealis.proto;

message ReturnValueTerm {
    extend borealis.proto.Term {
        optional ReturnValueTerm ext = $COUNTER_TERM;
    }

    optional string funcName = 1;
}

**/
class ReturnValueTerm: public borealis::Term {

    std::string funcName;

    ReturnValueTerm(Type::Ptr type, const std::string& funcName);

public:

    MK_COMMON_TERM_IMPL(ReturnValueTerm);

    const std::string& getFunctionName() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, ReturnValueTerm> {
    static Dynamic<Impl> doit(
            const ReturnValueTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getVarByTypeAndName(t->getType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* RETURNVALUETERM_H_ */

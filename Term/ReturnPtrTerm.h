/*
 * ReturnPtrTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef RETURNPTRTERM_H_
#define RETURNPTRTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/ReturnPtrTerm.proto
import "Term/Term.proto";

package borealis.proto;

message ReturnPtrTerm {
    extend borealis.proto.Term {
        optional ReturnPtrTerm ext = $COUNTER_TERM;
    }

    optional string funcName = 1;
}

**/
class ReturnPtrTerm: public borealis::Term {

    std::string funcName;

    ReturnPtrTerm(Type::Ptr type, const std::string& funcName);

public:

    MK_COMMON_TERM_IMPL(ReturnPtrTerm);

    const std::string& getFunctionName() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, ReturnPtrTerm> {
    static Dynamic<Impl> doit(
            const ReturnPtrTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getVarByTypeAndName(t->getType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* RETURNPTRTERM_H_ */

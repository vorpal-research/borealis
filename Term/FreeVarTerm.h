/*
 * FreeVarTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef FREEVARTERM_H_
#define FREEVARTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/FreeVarTerm.proto
import "Term/Term.proto";

package borealis.proto;

message FreeVarTerm {
    extend borealis.proto.Term {
        optional FreeVarTerm ext = $COUNTER_TERM;
    }
}

**/
class FreeVarTerm: public borealis::Term {
    std::string vname;
    bool global;

    FreeVarTerm(Type::Ptr type, const std::string& vname);

public:

    MK_COMMON_TERM_IMPL(FreeVarTerm);

    const std::string& getVName() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

template<class Impl>
struct SMTImpl<Impl, FreeVarTerm> {
    static Dynamic<Impl> doit(
            const FreeVarTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getVarByTypeAndName(t->getType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* FREEVARTERM_H_ */

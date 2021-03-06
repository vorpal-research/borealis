/*
 * ConstTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef CONSTTERM_H_
#define CONSTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/ConstTerm.proto
import "Term/Term.proto";

package borealis.proto;

message ConstTerm {
    extend borealis.proto.Term {
        optional ConstTerm ext = $COUNTER_TERM;
    }
}

**/
class ConstTerm: public borealis::Term {

    ConstTerm(Type::Ptr type, const std::string& name);

public:

    MK_COMMON_TERM_IMPL(ConstTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, ConstTerm> {
    static Dynamic<Impl> doit(
            const ConstTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {

        TRACE_FUNC;

        using borealis::logging::endl;
        using borealis::logging::wtf;
        // XXX: ConstTerm means we have an unsupported LLVM constant somewhere...
        auto&& res = ef.getVarByTypeAndName(t->getType(), t->getName());
        wtf() << "Got " << res << " for " << t->getName() << endl;
        return res;
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* CONSTTERM_H_ */

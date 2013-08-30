/*
 * ConstTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef CONSTTERM_H_
#define CONSTTERM_H_

#include "Protobuf/Gen/Term/ConstTerm.pb.h"

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/ConstTerm.proto
import "Term/Term.proto";

package borealis.proto;

message ConstTerm {
    extend borealis.proto.Term {
        optional ConstTerm ext = 19;
    }

    optional string asString = 1;
}

**/
class ConstTerm: public borealis::Term {

    util::option<std::string> asString;

    ConstTerm(Type::Ptr type, const std::string& name, util::option<std::string> asString) :
        Term(
                class_tag(*this),
                type,
                name
        ), asString(asString) {};

public:

    MK_COMMON_TERM_IMPL(ConstTerm);

    util::option<std::string> getAsString() const { return asString; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, ConstTerm> {
    static Dynamic<Impl> doit(
            const ConstTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        using borealis::logging::endl;
        using borealis::logging::wtf;
        // ConstTerm means we have an unsupported LLVM constant somewhere...
        auto res = ef.getVarByTypeAndName(t->getType(), t->getName());
        wtf() << "Got " << res << " for " << t->getName() << endl;
        return res;
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* CONSTTERM_H_ */

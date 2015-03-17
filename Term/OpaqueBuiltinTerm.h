/*
 * OpaqueBuiltinTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEBUILTINTERM_H_
#define OPAQUEBUILTINTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueBuiltinTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueBuiltinTerm {
    extend borealis.proto.Term {
        optional OpaqueBuiltinTerm ext = $COUNTER_TERM;
    }

    optional string vname = 1;
}

**/
class OpaqueBuiltinTerm: public borealis::Term {

    std::string vname;

    OpaqueBuiltinTerm(Type::Ptr type, const std::string& vname);

public:

    MK_COMMON_TERM_IMPL(OpaqueBuiltinTerm);

    const std::string& getVName() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueBuiltinTerm> {
    static Dynamic<Impl> doit(
            const OpaqueBuiltinTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* OPAQUEBUILTINTERM_H_ */

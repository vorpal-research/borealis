/*
 * OpaqueNamedConstantTerm.h
 *
 *  Created on: Sep 29, 2015
 *      Author: belyaev
 */

#ifndef OPAQUENAMEDCONSTANTTERM_H_
#define OPAQUENAMEDCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueNamedConstantTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueNamedConstantTerm {
    extend borealis.proto.Term {
        optional OpaqueNamedConstantTerm ext = $COUNTER_TERM;
    }

    optional string vname = 1;
}

**/
class OpaqueNamedConstantTerm: public borealis::Term {

    std::string vname;

    OpaqueNamedConstantTerm(Type::Ptr type, const std::string& vname);

public:

    MK_COMMON_TERM_IMPL(OpaqueNamedConstantTerm);

    const std::string& getVName() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueNamedConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueNamedConstantTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* OPAQUENAMEDCONSTANTTERM_H_ */

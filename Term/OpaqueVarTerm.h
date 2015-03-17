/*
 * OpaqueVarTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEVARTERM_H_
#define OPAQUEVARTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueVarTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueVarTerm {
    extend borealis.proto.Term {
        optional OpaqueVarTerm ext = $COUNTER_TERM;
    }

    optional string vname = 1;
}

**/
class OpaqueVarTerm: public borealis::Term {

    std::string vname;

    OpaqueVarTerm(Type::Ptr type, const std::string& vname);

public:

    MK_COMMON_TERM_IMPL(OpaqueVarTerm);

    const std::string& getVName() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueVarTerm> {
    static Dynamic<Impl> doit(
            const OpaqueVarTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* OPAQUEVARTERM_H_ */

/*
 * OpaqueConstantTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUESTRINGCONSTANTTERM_H_
#define OPAQUESTRINGCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueStringConstantTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueStringConstantTerm {
    extend borealis.proto.Term {
        optional OpaqueStringConstantTerm ext = $COUNTER_TERM;
    }

    optional string value = 1;
}

**/
class OpaqueStringConstantTerm: public borealis::Term {

    std::string value;

    OpaqueStringConstantTerm(Type::Ptr type, const std::string& value);

public:

    MK_COMMON_TERM_IMPL(OpaqueStringConstantTerm);

    const std::string& getValue() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueStringConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueStringConstantTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* OPAQUESTRINGCONSTANTTERM_H_ */

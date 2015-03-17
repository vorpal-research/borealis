/*
 * OpaqueConstantTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEFLOATINGCONSTANTTERM_H_
#define OPAQUEFLOATINGCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueFloatingConstantTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueFloatingConstantTerm {
    extend borealis.proto.Term {
        optional OpaqueFloatingConstantTerm ext = $COUNTER_TERM;
    }

    optional double value = 1;
}

**/
class OpaqueFloatingConstantTerm: public borealis::Term {

    static constexpr double EPS = 4 * std::numeric_limits<double>::epsilon();

    double value;

    OpaqueFloatingConstantTerm(Type::Ptr type, double value);

public:

    MK_COMMON_TERM_IMPL(OpaqueFloatingConstantTerm);

    double getValue() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

    // XXX: akhin Do we need this or string-based equality is good enough?
    virtual bool equals(const Term* other) const override;
    virtual size_t hashCode() const override;

};

template<class Impl>
struct SMTImpl<Impl, OpaqueFloatingConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueFloatingConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        return ef.getRealConst(t->getValue());
    }
};

} /* namespace borealis */

#endif /* OPAQUEFLOATINGCONSTANTTERM_H_ */

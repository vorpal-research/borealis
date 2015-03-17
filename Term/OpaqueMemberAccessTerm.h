/*
 * OpaqueMemberAccessTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEMEMBERACCESSTERM_H_
#define OPAQUEMEMBERACCESSTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueMemberAccessTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueMemberAccessTerm {
    extend borealis.proto.Term {
        optional OpaqueMemberAccessTerm ext = $COUNTER_TERM;
    }

    optional Term lhv = 1;
    optional string property = 2;
    optional bool indirect = 3;
}

**/
class OpaqueMemberAccessTerm: public borealis::Term {

    std::string property;
    bool indirect;

    OpaqueMemberAccessTerm(Type::Ptr type, Term::Ptr lhv, const std::string& property, bool indirect = false);

public:

    MK_COMMON_TERM_IMPL(OpaqueMemberAccessTerm);

    Term::Ptr getLhv() const;
    const std::string& getProperty() const;
    bool isIndirect() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _lhv = tr->transform(getLhv());
        auto&& _type = type;
        TERM_ON_CHANGED(
            getLhv() != _lhv,
            new Self( _type, _lhv, property, indirect )
        );
    }

    virtual bool equals(const Term* other) const override;
    virtual size_t hashCode() const override;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueMemberAccessTerm> {
    static Dynamic<Impl> doit(
            const OpaqueMemberAccessTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} // namespace borealis

#endif /* OPAQUEMEMBERACCESSTERM_H_ */

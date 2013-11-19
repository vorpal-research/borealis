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
    optional bool isIndirect = 3;
}

**/
class OpaqueMemberAccessTerm: public borealis::Term {
    Term::Ptr lhv;
    std::string property;
    bool isIndirect;

    OpaqueMemberAccessTerm(Type::Ptr type, Term::Ptr lhv, const std::string& property, bool isIndirect = false):
        Term(
            class_tag(*this),
            type,
            lhv->getName() + (isIndirect ? "->" : ".") + property
        ), lhv(lhv), property(property), isIndirect(isIndirect) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueMemberAccessTerm);

    Term::Ptr getLhv() const { return lhv; }
    const std::string& getProperty() const { return property; }
    bool getIsIndirect() const { return isIndirect; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        auto _lhv = tr->transform(lhv);
        return new Self{ getType(), _lhv, property, isIndirect };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->lhv == *lhv &&
                    that->property == property &&
                    that->isIndirect == isIndirect;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), lhv, property, isIndirect);
    }
};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueMemberAccessTerm> {
    static Dynamic<Impl> doit(
            const OpaqueMemberAccessTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        USING_SMT_IMPL(Impl);
        BYE_BYE(Dynamic, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} // namespace borealis

#endif /* OPAQUEMEMBERACCESSTERM_H_ */

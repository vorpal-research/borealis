/*
 * SignTerm.h
 *
 *  Created on: Aug 28, 2013
 *      Author: sam
 */

#ifndef SIGNTERM_H_
#define SIGNTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/SignTerm.proto
import "Term/Term.proto";

package borealis.proto;

message SignTerm {
    extend borealis.proto.Term {
        optional SignTerm ext = $COUNTER_TERM;
    }

    optional Term rhv = 1;
}

**/
class SignTerm: public borealis::Term {

    Term::Ptr rhv;

    SignTerm(Type::Ptr type, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            "sign(" + rhv->getName() + ")"
        ), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(SignTerm);

    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto _rhv = tr->transform(rhv);
        auto _type = getTermType(tr->FN.Type, _rhv);
        TERM_ON_CHANGED(
            rhv != _rhv,
            new Self( _type, _rhv )
        );
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->rhv == *rhv;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), rhv);
    }

    static Type::Ptr getTermType(TypeFactory::Ptr TyF, Term::Ptr rhv) {
        auto type = rhv->getType();

        if (!TyF->isValid(type)) return type;

        if (llvm::isa<type::Integer>(type) || llvm::isa<type::Float>(type)) {
            return TyF->getInteger();
        } else {
            return TyF->getTypeError("Sign for non-numerical: " + TyF->toString(*type));
        }
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, SignTerm> {
    static Dynamic<Impl> doit(
            const SignTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        USING_SMT_IMPL(Impl);

        auto rhvsmt = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        auto rhvbv = rhvsmt.template to<DynBV>();
        ASSERT(!rhvbv.empty(), "Sign for non bit-vector");

        auto rhv = rhvbv.getUnsafe();
        auto size = rhv.getBitSize();

        return rhv.extract(size-1, size-1).zgrowTo(Integer::bitsize);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* SIGNTERM_H_ */

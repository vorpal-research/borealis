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

    SignTerm(Type::Ptr type, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(SignTerm);

    Term::Ptr getRhv() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _rhv = tr->transform(getRhv());
        TERM_ON_CHANGED(
            getRhv() != _rhv,
            tr->FN.Term->getSignTerm(_rhv)
        );
    }

    static Type::Ptr getTermType(TypeFactory::Ptr TyF, Term::Ptr rhv) {
        auto&& type = rhv->getType();

        if (TypeUtils::isInvalid(type)) return type;

        if (llvm::isa<type::Integer>(type) || llvm::isa<type::Float>(type)) {
            return TyF->getInteger(1);
        } else {
            return TyF->getTypeError("Sign for non-numerical: " + util::toString(*type));
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

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        DynBV rhv = SMT<Impl>::doit(t->getRhv(), ef, ctx);
        ASSERT(rhv, "Sign for non bit-vector");

        auto&& size = rhv.getBitSize();
        return rhv.extract(size-1, size-1);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* SIGNTERM_H_ */

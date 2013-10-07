/*
 * LoadTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef LOADTERM_H_
#define LOADTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/LoadTerm.proto
import "Term/Term.proto";

package borealis.proto;

message LoadTerm {
    extend borealis.proto.Term {
        optional LoadTerm ext = $COUNTER_TERM;
    }

    optional Term rhv = 1;
}

**/
class LoadTerm: public borealis::Term {

    Term::Ptr rhv;

    LoadTerm(Type::Ptr type, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            "*(" + rhv->getName() + ")"
        ), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(LoadTerm);

    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        auto _rhv = tr->transform(rhv);
        auto _type = getTermType(tr->FN.Type, _rhv);
        return new Self{ _type, _rhv };
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

        if (auto* ptr = llvm::dyn_cast<type::Pointer>(type)) {
            return ptr->getPointed();
        } else {
            return TyF->getTypeError(
                "Load from a non-pointer: " + TyF->toString(*type)
            );
        }
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, LoadTerm> {
    static Dynamic<Impl> doit(
            const LoadTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        auto r = SMT<Impl>::doit(t->getRhv(), ef, ctx).template to<Pointer>();
        ASSERT(!r.empty(), "Load with non-pointer right side");
        auto rp = r.getUnsafe();

        return ctx->readExprFromMemory(rp, ExprFactory::sizeForType(t->getType()));
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* LOADTERM_H_ */

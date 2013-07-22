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

class LoadTerm: public borealis::Term {

    Term::Ptr rhv;

    LoadTerm(Term::Ptr rhv):
        Term(
                rhv->hashCode(),
                "*(" + rhv->getName() + ")",
                type_id(*this)
        ), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(LoadTerm);

    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        return new Self{ tr->transform(rhv) };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->rhv == *rhv;
        } else return false;
    }

    virtual Type::Ptr getTermType() const override {
        auto& tf = TypeFactory::getInstance();
        auto ptr = rhv->getTermType();

        if (!tf.isValid(ptr)) return ptr;

        if (auto* cst = llvm::dyn_cast<type::Pointer>(ptr)) {
            return cst->getPointed();
        } else {
            return tf.getTypeError("Load from a non-pointer");
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

        return ctx->readExprFromMemory(rp, ExprFactory::sizeForType(t->getTermType()));
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* LOADTERM_H_ */

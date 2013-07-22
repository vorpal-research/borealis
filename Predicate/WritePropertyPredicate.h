/*
 * WritePropertyPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef WRITEPROPERTYPREDICATE_H_
#define WRITEPROPERTYPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class WritePropertyPredicate: public borealis::Predicate {

    Term::Ptr propName;
    Term::Ptr lhv;
    Term::Ptr rhv;

    WritePropertyPredicate(
            Term::Ptr propName,
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(WritePropertyPredicate);

    Term::Ptr getPropertyName() const { return propName; }
    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class SubClass>
    const Self* accept(Transformer<SubClass>* t) const {
        return new Self{
            t->transform(propName),
            t->transform(lhv),
            t->transform(rhv),
            this->type
        };
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, WritePropertyPredicate> {
    static Bool<Impl> doit(
            const WritePropertyPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        ASSERT(llvm::isa<ConstTerm>(p->getPropertyName()),
               "Property write with non-constant property name");
        auto* constPropName = llvm::cast<ConstTerm>(p->getPropertyName());
        auto strPropName = getAsCompileTimeString(constPropName->getConstant());
        ASSERT(!strPropName.empty(),
               "Property write with unknown property name");

        auto l = SMT<Impl>::doit(p->getLhv(), ef, ctx).template to<Pointer>();
        ASSERT(!l.empty(), "Property write with a non-pointer value");
        auto lp = l.getUnsafe();

        auto r = SMT<Impl>::doit(p->getRhv(), ef, ctx);

        ctx->writeProperty(strPropName.getUnsafe(), lp, r);

        return ef.getTrue();
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* WRITEPROPERTYPREDICATE_H_ */

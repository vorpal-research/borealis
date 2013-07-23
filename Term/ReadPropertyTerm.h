/*
 * ReadPropertyTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef READPROPERTYTERM_H_
#define READPROPERTYTERM_H_

#include "Codegen/llvm.h"
#include "Term/ConstTerm.h"
#include "Term/Term.h"

namespace borealis {

class ReadPropertyTerm: public borealis::Term {

    Term::Ptr propName;
    Term::Ptr rhv;
    Type::Ptr type;

    ReadPropertyTerm(Term::Ptr propName, Term::Ptr rhv, Type::Ptr type):
        Term(
            util::hash::defaultHasher()(propName, rhv, type),
            "read(" + propName->getName() + "," + rhv->getName() + ")",
            type_id(*this)
        ), propName(propName), rhv(rhv), type(type) {};

public:

    MK_COMMON_TERM_IMPL(ReadPropertyTerm);

    Term::Ptr getPropertyName() const { return propName; }
    Term::Ptr getRhv() const { return rhv; }
    Type::Ptr getType() const { return type; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        return new Self{ tr->transform(propName), tr->transform(rhv), type };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->propName == *propName &&
                    *that->rhv == *rhv &&
                    that->type == type;
        } else return false;
    }

    virtual Type::Ptr getTermType() const override {
        return type;
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, ReadPropertyTerm> {
    static Dynamic<Impl> doit(
            const ReadPropertyTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        ASSERT(llvm::isa<ConstTerm>(t->getPropertyName()),
               "Property read with non-constant property name");
        auto* constPropName = llvm::cast<ConstTerm>(t->getPropertyName());
        auto strPropName = getAsCompileTimeString(constPropName->getConstant());
        ASSERT(!strPropName.empty(),
               "Property read with unknown property name");

        auto r = SMT<Impl>::doit(t->getRhv(), ef, ctx).template to<Pointer>();
        ASSERT(!r.empty(), "Property read with non-pointer right side");
        auto rp = r.getUnsafe();

        return ctx->readProperty(strPropName.getUnsafe(), rp, ExprFactory::sizeForType(t->getType()));
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* READPROPERTYTERM_H_ */

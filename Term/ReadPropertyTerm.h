/*
 * ReadPropertyTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef READPROPERTYTERM_H_
#define READPROPERTYTERM_H_

#include "Term/ConstTerm.h"
#include "Term/Term.h"

namespace borealis {

class ReadPropertyTerm: public borealis::Term {

    Term::Ptr propName;
    Term::Ptr rhv;

    ReadPropertyTerm(Type::Ptr type, Term::Ptr propName, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            "read(" + propName->getName() + "," + rhv->getName() + ")"
        ), propName(propName), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(ReadPropertyTerm);

    Term::Ptr getPropertyName() const { return propName; }
    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        auto _propName = tr->transform(propName);
        auto _rhv = tr->transform(rhv);
        auto _type = type;
        return new Self{ _type, _propName, _rhv };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->propName == *propName &&
                    *that->rhv == *rhv;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), propName, rhv);
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
        auto strPropName = constPropName->getAsString();
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

/*
 * ReadPropertyTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef READPROPERTYTERM_H_
#define READPROPERTYTERM_H_

#include "Term/Term.h"

namespace borealis {

class ReadPropertyTerm: public borealis::Term {

    typedef ReadPropertyTerm self;

    std::string propName;
    Term::Ptr rhv;
    Type::Ptr type;

    ReadPropertyTerm(const std::string propName, Term::Ptr rhv, Type::Ptr type):
        Term(
                std::hash<std::string>()(propName) ^ rhv->hashCode() ^ type->getId(),
                "read(" + propName + "," + rhv->getName() + ")",
                type_id(*this)
        ), propName(propName), rhv(rhv), type(type) {};

public:

    ReadPropertyTerm(const ReadPropertyTerm&) = default;
    ~ReadPropertyTerm();

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new ReadPropertyTerm(propName, tr->transform(rhv), type);
    }

#include "Util/macros.h"
    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const override {
        typedef Z3ExprFactory::Dynamic Dynamic;
        typedef Z3ExprFactory::Pointer Pointer;

        ASSERTC(ctx != nullptr);

        Dynamic r = rhv->toZ3(z3ef, ctx);
        ASSERT(r.is<Pointer>(),
               "Encountered property read with non-pointer right side");

        auto rp = r.to<Pointer>().getUnsafe();

        return ctx->readProperty(propName, rp, Z3ExprFactory::sizeForType(type));
    }
#include "Util/unmacros.h"

    virtual bool equals(const Term* other) const override {
        if (const self* that = llvm::dyn_cast<self>(other)) {
            return  Term::equals(other) &&
                    that->propName == propName &&
                    *that->rhv == *rhv &&
                    *that->type == *type;
        } else return false;
    }

    const std::string& getPropertyName() const { return propName; }
    Term::Ptr getRhv() const { return rhv; }
    Type::Ptr getType() const { return type; }

    static bool classof(const self*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    virtual Type::Ptr getTermType() const override {
        return type;
    }

    friend class TermFactory;
};

} /* namespace borealis */

#endif /* READPROPERTYTERM_H_ */

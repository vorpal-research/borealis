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
    typedef LoadTerm self;

    Term::Ptr rhv;

    LoadTerm(Term::Ptr rhv):
        Term(
                rhv->getId(),
                llvm::ValueType::PTR_VAR, // FIXME: infer the correct type?
                "*(" + rhv->getName() + ")",
                type_id(*this)
        ), rhv(rhv){};

public:

    LoadTerm(const LoadTerm&) = default;
    ~LoadTerm();

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new LoadTerm(tr->transform(rhv));
    }

#include "Util/macros.h"
    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx = nullptr) const {
        typedef Z3ExprFactory::Pointer Pointer;
        typedef Z3ExprFactory::Dynamic Dynamic;

        Dynamic r = z3ef.getExprForTerm(*rhv);
        if (!r.is<Pointer>()) {
            BYE_BYE(logic::Bool, "Encountered load with non-pointer right side");
        }

        auto rp = r.to<Pointer>().getUnsafe();

        if (ctx) {
            return z3ef.if_(z3ef.isInvalidPtrExpr(rp))
                       .then_<Z3ExprFactory::Dynamic>(z3ef.getInvalidPtr())
                       .else_(ctx->readExprFromMemory(rp, Z3ExprFactory::sizeForType(getTermType())));
        }

        return z3ef.getTrue();
    }
#include "Util/unmacros.h"

    virtual bool equals(const Term* other) const {
        if (const LoadTerm* that = llvm::dyn_cast<LoadTerm>(other)) {
            return  Term::equals(other) &&
                    *that->rhv == *rhv;
        } else return false;
    }

    Term::Ptr getRhv() const { return rhv; }

    static bool classof(const LoadTerm*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    virtual Type::Ptr getTermType() const {
        Type::Ptr ptr = rhv->getTermType();

        // if we are in an error, return it
        if(!TypeFactory::getInstance().isValid(ptr)) return ptr;

        if(auto* cst = llvm::dyn_cast<Pointer>(ptr)) {
            return cst->getPointed();
        } else {
            return TypeFactory::getInstance().getTypeError("Load from a non-pointer");
        }
    }


    friend class TermFactory;
};

} /* namespace borealis */

#endif /* LOADTERM_H_ */

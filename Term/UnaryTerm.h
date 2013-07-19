/*
 * UnaryTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef UNARYTERM_H_
#define UNARYTERM_H_

#include "Term/Term.h"

namespace borealis {

class UnaryTerm: public borealis::Term {

    typedef UnaryTerm Self;

    llvm::UnaryArithType opcode;
    Term::Ptr rhv;

    UnaryTerm(llvm::UnaryArithType opcode, Term::Ptr rhv):
        Term(
            rhv->hashCode(),
            llvm::unaryArithString(opcode) + "(" + rhv->getName() + ")",
            type_id(*this)
        ), opcode(opcode), rhv(rhv) {};

public:

    UnaryTerm(const Self&) = default;
    virtual ~UnaryTerm() {};

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        return new UnaryTerm(opcode, tr->transform(rhv));
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    that->opcode == opcode &&
                    *that->rhv == *rhv;
        } else return false;
    }

    llvm::UnaryArithType getOpcode() const { return opcode; }
    Term::Ptr getRhv() const { return rhv; }

    static bool classof(const UnaryTerm*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<Self>();
    }

    virtual Type::Ptr getTermType() const override {
        return rhv->getTermType();
    }

    friend class TermFactory;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, UnaryTerm> {
    static Dynamic<Impl> doit(
            const UnaryTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        USING_SMT_IMPL(Impl);

        auto rhvz3 = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        switch(t->getOpcode()) {
        case llvm::UnaryArithType::BNOT: {
            auto rhvi = rhvz3.template to<Integer>();
            ASSERT(!rhvi.empty(), "Bit not: rhv is not an integer");
            return ~rhvi.getUnsafe();
        }
        case llvm::UnaryArithType::NEG: {
            auto rhvi = rhvz3.template to<Integer>();
            ASSERT(!rhvi.empty(), "Negate: rhv is not an integer");
            return -rhvi.getUnsafe();
        }
        case llvm::UnaryArithType::NOT: {
            auto rhvi = rhvz3.template to<Integer>();
            ASSERT(!rhvi.empty(), "Logic not: rhv is not a boolean");
            return !rhvi.getUnsafe();
        }
        }
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* UNARYTERM_H_ */

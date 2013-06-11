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

    typedef UnaryTerm self;

    llvm::UnaryArithType opcode;
    Term::Ptr rhv;

    UnaryTerm(llvm::UnaryArithType opcode, Term::Ptr rhv):
        Term(
                rhv->hashCode(),
                llvm::unaryArithString(opcode) + "(" + rhv->getName() + ")",
                type_id(*this)
        ), opcode(opcode), rhv(rhv) {};

public:

    UnaryTerm(const UnaryTerm&) = default;
    ~UnaryTerm();

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new UnaryTerm(opcode, tr->transform(rhv));
    }

#include "Util/macros.h"
    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const override {
        typedef Z3ExprFactory::Bool Bool;
        typedef Z3ExprFactory::Integer Int;

        auto z3rhv = rhv->toZ3(z3ef, ctx);

        switch(opcode) {
        case llvm::UnaryArithType::BNOT:
            ASSERT(z3rhv.is<Int>(), "Bit not: rhv is not an integer");
            return ~z3rhv.to<Int>().getUnsafe();
        case llvm::UnaryArithType::NEG:
            ASSERT(z3rhv.is<Int>(), "Negate: rhv is not an integer");
            return -z3rhv.to<Int>().getUnsafe();
        case llvm::UnaryArithType::NOT:
            ASSERT(z3rhv.is<Bool>(), "Logic not: rhv is not a boolean");
            return !z3rhv.to<Bool>().getUnsafe();
        }
    }
#include "Util/unmacros.h"

    virtual bool equals(const Term* other) const override {
        if (const UnaryTerm* that = llvm::dyn_cast<UnaryTerm>(other)) {
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
        return t->getTermTypeId() == type_id<self>();
    }

    virtual Type::Ptr getTermType() const override {
        return rhv->getTermType();
    }

    friend class TermFactory;
};

} /* namespace borealis */

#endif /* UNARYTERM_H_ */

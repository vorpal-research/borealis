/*
 * BinaryTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef BINARYTERM_H_
#define BINARYTERM_H_

#include "Term/Term.h"

namespace borealis {

class BinaryTerm: public borealis::Term {
    typedef BinaryTerm self;

    llvm::ArithType opcode;
    Term::Ptr lhv;
    Term::Ptr rhv;

    BinaryTerm(llvm::ArithType opcode, Term::Ptr lhv, Term::Ptr rhv):
        Term(
                lhv->getId() ^ rhv->getId(),
                llvm::ValueType::INT_VAR, // FIXME: infer the correct type?
                lhv->getName() + llvm::arithString(opcode) + rhv->getName(),
                type_id(*this)
        ), opcode(opcode), lhv(lhv), rhv(rhv) {};

public:

    BinaryTerm(const BinaryTerm&) = default;
    ~BinaryTerm();

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new BinaryTerm(opcode, tr->transform(lhv), tr->transform(rhv));
    }

#include "Util/macros.h"
    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx = nullptr) const {
        BYE_BYE(Z3ExprFactory::Dynamic, "Unsupported")
    }
#include "Util/unmacros.h"

    virtual bool equals(const Term* other) const {
        if (const BinaryTerm* that = llvm::dyn_cast<BinaryTerm>(other)) {
            return  Term::equals(other) &&
                    that->opcode == opcode &&
                    *that->lhv == *lhv &&
                    *that->rhv == *rhv;
        } else return false;
    }

    Term::Ptr getLhv() { return lhv; }
    Term::Ptr getRhv() { return rhv; }

    static bool classof(const BinaryTerm*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    virtual Type::Ptr getTermType() const {
        auto& tf = TypeFactory::getInstance();

        if(!tf.isValid(rhv->getTermType())) return rhv->getTermType();
        if(!tf.isValid(lhv->getTermType())) return lhv->getTermType();

        if(rhv->getTermType() != lhv->getTermType()) {
            return tf.getTypeError("Invalid binop: types do not correspond");
        }

        return rhv->getTermType();
    }

    friend class TermFactory;
};

} /* namespace borealis */

#endif /* BINARYTERM_H_ */

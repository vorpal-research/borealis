/*
 * UnaryTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef UNARYTERM_H_
#define UNARYTERM_H_

#include "Term.h"

namespace borealis {

class UnaryTerm: public Term {
    typedef UnaryTerm self;

    llvm::UnaryArithType opcode;
    Term::Ptr rhv;

    UnaryTerm(llvm::UnaryArithType opcode, Term::Ptr rhv):
        Term(
                rhv->getId(),
                llvm::ValueType::INT_VAR, // FIXME
                llvm::unaryArithString(opcode) + "(" + rhv->getName() + ")",
                type_id(*this)
        ), opcode(opcode), rhv(rhv){};

public:
    UnaryTerm(const UnaryTerm&) = default;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new UnaryTerm(opcode, tr->transform(rhv));
    }

    ~UnaryTerm();

    virtual bool equals(const Term* other) const {
        if(const UnaryTerm* that = llvm::dyn_cast<UnaryTerm>(other)) {
            return  Term::equals(other) &&
                    that->opcode == opcode &&
                    that->rhv == rhv;
        } else return false;
    }

    Term::Ptr getRhv() const { return rhv; }

    static bool classof(const UnaryTerm*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    friend class TermFactory;
};

} /* namespace borealis */
#endif /* UNARYTERM_H_ */

/*
 * CmpTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef CMPTERM_H_
#define CMPTERM_H_

#include "Term.h"

namespace borealis {

class CmpTerm: public Term {
    typedef CmpTerm self;

    llvm::ConditionType opcode;
    Term::Ptr lhv;
    Term::Ptr rhv;

    CmpTerm(llvm::ConditionType opcode, Term::Ptr lhv, Term::Ptr rhv):
        Term(
                lhv->getId() ^ rhv->getId(),
                llvm::ValueType::INT_VAR, // FIXME
                lhv->getName() + llvm::conditionString(opcode) + rhv->getName(),
                type_id(*this)
        ), opcode(opcode), lhv(lhv), rhv(rhv){};

public:
    CmpTerm(const CmpTerm&) = default;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new CmpTerm(opcode, tr->transform(lhv), tr->transform(rhv));
    }

    ~CmpTerm();

    virtual bool equals(const Term* other) const {
        if(const CmpTerm* that = llvm::dyn_cast<CmpTerm>(other)) {
            return  Term::equals(other) &&
                    that->opcode == opcode &&
                    that->lhv == lhv &&
                    that->rhv == rhv;
        } else return false;
    }

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    static bool classof(const CmpTerm*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    friend class TermFactory;
};

} /* namespace borealis */
#endif /* CMPTERM_H_ */

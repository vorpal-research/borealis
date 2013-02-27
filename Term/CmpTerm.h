/*
 * CmpTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef CMPTERM_H_
#define CMPTERM_H_

#include "Term/Term.h"

namespace borealis {

class CmpTerm: public borealis::Term {
    typedef CmpTerm self;

    llvm::ConditionType opcode;
    Term::Ptr lhv;
    Term::Ptr rhv;

    CmpTerm(llvm::ConditionType opcode, Term::Ptr lhv, Term::Ptr rhv):
        Term(
                lhv->getId() ^ rhv->getId(),
                llvm::ValueType::BOOL_VAR, // FIXME: infer the correct type?
                lhv->getName() + llvm::conditionString(opcode) + rhv->getName(),
                type_id(*this)
        ), opcode(opcode), lhv(lhv), rhv(rhv) {};

public:

    CmpTerm(const CmpTerm&) = default;
    ~CmpTerm();

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new CmpTerm(opcode, tr->transform(lhv), tr->transform(rhv));
    }

#include "Util/macros.h"
    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx = nullptr) const {
        auto lhvz3 = lhv->toZ3(z3ef, ctx);
        auto rhvz3 = rhv->toZ3(z3ef, ctx);
        if(!lhvz3.isComparable() || !rhvz3.isComparable()) {
            BYE_BYE(Z3ExprFactory::Dynamic, "Comparing uncomparable expressions")
        }

        auto lhv = lhvz3.toComparable().getUnsafe();
        auto rhv = rhvz3.toComparable().getUnsafe();

        switch(opcode) {
        case llvm::ConditionType::EQ:    return lhv == rhv;
        case llvm::ConditionType::FALSE: return z3ef.getFalse();
        case llvm::ConditionType::GT:    return lhv >  rhv;
        case llvm::ConditionType::GTE:   return lhv >= rhv;
        case llvm::ConditionType::LT:    return lhv <  rhv;
        case llvm::ConditionType::LTE:   return lhv <= rhv;
        case llvm::ConditionType::NEQ:   return lhv != rhv;
        case llvm::ConditionType::TRUE:  return z3ef.getTrue();
        default: BYE_BYE(Z3ExprFactory::Dynamic, "Unsupported opcode")
        }
    }
#include "Util/unmacros.h"

    virtual bool equals(const Term* other) const {
        if (const CmpTerm* that = llvm::dyn_cast<CmpTerm>(other)) {
            return  Term::equals(other) &&
                    that->opcode == opcode &&
                    *that->lhv == *lhv &&
                    *that->rhv == *rhv;
        } else return false;
    }

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    virtual Type::Ptr getTermType() const {
        auto& tf = TypeFactory::getInstance();

        if(!tf.isValid(rhv->getTermType())) return rhv->getTermType();
        if(!tf.isValid(lhv->getTermType())) return lhv->getTermType();

        if(rhv->getTermType() != lhv->getTermType()) {
            return tf.getTypeError("Invalid cmp: types do not correspond");
        }

        return tf.getBool();
    }

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

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

    llvm::ConditionType opcode;
    Term::Ptr lhv;
    Term::Ptr rhv;

    CmpTerm(llvm::ConditionType opcode, Term::Ptr lhv, Term::Ptr rhv):
        Term(
                lhv->hashCode() ^ rhv->hashCode() ^ std::hash<llvm::ConditionType>()(opcode),
                "(" + lhv->getName() + " " + llvm::conditionString(opcode) + " " + rhv->getName() + ")",
                type_id(*this)
        ), opcode(opcode), lhv(lhv), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(CmpTerm);

    llvm::ConditionType getOpcode() const { return opcode; }
    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        return new Self{ opcode, tr->transform(lhv), tr->transform(rhv) };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    that->opcode == opcode &&
                    *that->lhv == *lhv &&
                    *that->rhv == *rhv;
        } else return false;
    }

    virtual Type::Ptr getTermType() const override {
        auto& tf = TypeFactory::getInstance();

        if (!tf.isValid(rhv->getTermType())) return rhv->getTermType();
        if (!tf.isValid(lhv->getTermType())) return lhv->getTermType();

        if (rhv->getTermType() != lhv->getTermType()) {
            return tf.getTypeError("Invalid cmp: types do not correspond");
        }

        return tf.getBool();
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, CmpTerm> {
    static Dynamic<Impl> doit(
            const CmpTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        USING_SMT_IMPL(Impl)

        auto lhvz3 = SMT<Impl>::doit(t->getLhv(), ef, ctx);
        auto rhvz3 = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        ASSERT(lhvz3.isComparable() && rhvz3.isComparable(),
               "Comparing incomparable expressions");

        auto lhv = lhvz3.toComparable().getUnsafe();
        auto rhv = rhvz3.toComparable().getUnsafe();

        switch(t->getOpcode()) {
        case llvm::ConditionType::EQ:    return lhv == rhv;
        case llvm::ConditionType::NEQ:   return lhv != rhv;

        case llvm::ConditionType::GT:    return lhv >  rhv;
        case llvm::ConditionType::GE:    return lhv >= rhv;
        case llvm::ConditionType::LT:    return lhv <  rhv;
        case llvm::ConditionType::LE:    return lhv <= rhv;

        case llvm::ConditionType::UGT:   return lhv.ugt(rhv);
        case llvm::ConditionType::UGE:   return lhv.uge(rhv);
        case llvm::ConditionType::ULT:   return lhv.ult(rhv);
        case llvm::ConditionType::ULE:   return lhv.ule(rhv);

        case llvm::ConditionType::TRUE:  return ef.getTrue();
        case llvm::ConditionType::FALSE: return ef.getFalse();

        default: BYE_BYE(Dynamic, "Unsupported opcode");
        }
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* CMPTERM_H_ */

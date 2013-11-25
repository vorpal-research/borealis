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

/** protobuf -> Term/CmpTerm.proto
import "Term/Term.proto";
import "Util/ConditionType.proto";

package borealis.proto;

message CmpTerm {
    extend borealis.proto.Term {
        optional CmpTerm ext = $COUNTER_TERM;
    }

    optional ConditionType opcode = 1;
    optional Term lhv = 2;
    optional Term rhv = 3;
}

**/
class CmpTerm: public borealis::Term {

    llvm::ConditionType opcode;
    Term::Ptr lhv;
    Term::Ptr rhv;

    CmpTerm(Type::Ptr type, llvm::ConditionType opcode, Term::Ptr lhv, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            "(" + lhv->getName() + " " + llvm::conditionString(opcode) + " " + rhv->getName() + ")"
        ), opcode(opcode), lhv(lhv), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(CmpTerm);

    llvm::ConditionType getOpcode() const { return opcode; }
    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto _lhv = tr->transform(lhv);
        auto _rhv = tr->transform(rhv);
        auto _type = getTermType(tr->FN.Type, _lhv, _rhv);
        TERM_ON_CHANGED(
            lhv != _lhv || rhv != _rhv,
            new Self( _type, opcode, _lhv, _rhv )
        );
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    that->opcode == opcode &&
                    *that->lhv == *lhv &&
                    *that->rhv == *rhv;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), opcode, lhv, rhv);
    }

    static Type::Ptr getTermType(TypeFactory::Ptr TyF, Term::Ptr lhv, Term::Ptr rhv) {
        auto merged = TyF->merge(lhv->getType(), rhv->getType());

        if (TypeUtils::isValid(merged)) return TyF->getBool();
        else return merged;
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

        if (lhvz3.isComparable() && rhvz3.isComparable()) {
            auto lhv = lhvz3.toComparable().getUnsafe();
            auto rhv = rhvz3.toComparable().getUnsafe();

            switch(t->getOpcode()) {
            case llvm::ConditionType::EQ:    return lhv == rhv;
            case llvm::ConditionType::NEQ:   return lhv != rhv;

            case llvm::ConditionType::GT:    return lhv >  rhv;
            case llvm::ConditionType::GE:    return lhv >= rhv;
            case llvm::ConditionType::LT:    return lhv <  rhv;
            case llvm::ConditionType::LE:    return lhv <= rhv;

            case llvm::ConditionType::TRUE:  return ef.getTrue();
            case llvm::ConditionType::FALSE: return ef.getFalse();

            default: break; // fall-through and try to unsigned compare
            }
        }

        if (lhvz3.isUnsignedComparable() && rhvz3.isUnsignedComparable()) {
            auto lhv = lhvz3.toUnsignedComparable().getUnsafe();
            auto rhv = rhvz3.toUnsignedComparable().getUnsafe();

            switch(t->getOpcode()) {
            case llvm::ConditionType::UGT:   return lhv.ugt(rhv);
            case llvm::ConditionType::UGE:   return lhv.uge(rhv);
            case llvm::ConditionType::ULT:   return lhv.ult(rhv);
            case llvm::ConditionType::ULE:   return lhv.ule(rhv);
            default: break;
            }
        }

        BYE_BYE(Dynamic, "Unsupported CmpTerm: " + util::toString(t->getName()));
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* CMPTERM_H_ */

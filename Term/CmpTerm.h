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

    CmpTerm(Type::Ptr type, llvm::ConditionType opcode, Term::Ptr lhv, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(CmpTerm);

    llvm::ConditionType getOpcode() const;
    Term::Ptr getLhv() const;
    Term::Ptr getRhv() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _lhv = tr->transform(getLhv());
        auto&& _rhv = tr->transform(getRhv());
        TERM_ON_CHANGED(
            getLhv() != _lhv || getRhv() != _rhv,
            tr->FN.Term->getCmpTerm(opcode, _lhv, _rhv)
        );
    }

    virtual bool equals(const Term* other) const override;
    virtual size_t hashCode() const override;

    static Type::Ptr getTermType(TypeFactory::Ptr TyF, Term::Ptr lhv, Term::Ptr rhv) {
        auto&& merged = TyF->merge(lhv->getType(), rhv->getType());

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

        TRACE_FUNC;
        USING_SMT_IMPL(Impl)

        auto&& lhvz3 = SMT<Impl>::doit(t->getLhv(), ef, ctx);
        auto&& rhvz3 = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        switch(t->getOpcode()) {
            case llvm::ConditionType::TRUE:  return ef.getTrue();
            case llvm::ConditionType::FALSE: return ef.getFalse();
            default: break;
        }

        Bool lbool = lhvz3;
        Bool rbool = rhvz3;
        if(bool(lbool) && bool(rbool))
            switch(t->getOpcode()) {
            case llvm::ConditionType::EQ:    return lbool == rbool;
            case llvm::ConditionType::NEQ:   return lbool != rbool;
            default: break;
        }

        DynBV lbv = lhvz3;
        DynBV rbv = rhvz3;

        if (bool(lbv) && bool(rbv)) {
            switch (t->getOpcode()) {
            case llvm::ConditionType::EQ:    return lbv == rbv;
            case llvm::ConditionType::NEQ:   return lbv != rbv;
            case llvm::ConditionType::GT:    return lbv >  rbv;
            case llvm::ConditionType::GE:    return lbv >= rbv;
            case llvm::ConditionType::LT:    return lbv <  rbv;
            case llvm::ConditionType::LE:    return lbv <= rbv;
            case llvm::ConditionType::UGT:   return lbv.ugt(rbv);
            case llvm::ConditionType::UGE:   return lbv.uge(rbv);
            case llvm::ConditionType::ULT:   return lbv.ult(rbv);
            case llvm::ConditionType::ULE:   return lbv.ule(rbv);

            default: break; // fall-through and try to unsigned compare
            }
        }

        BYE_BYE(Dynamic, "Unsupported CmpTerm: " + t->getName());
    }

};
#include "Util/unmacros.h"

struct CmpTermExtractor {

    functional_hell::matchers::storage_t<llvm::ConditionType, Term::Ptr, Term::Ptr> unapply(Term::Ptr t) const {
        if (auto&& p = llvm::dyn_cast<CmpTerm>(t)) {
            return functional_hell::matchers::make_storage(p->getOpcode(), p->getLhv(), p->getRhv());
        } else {
            return {};
        }
    }

};

static auto $CmpTerm = functional_hell::matchers::make_pattern(CmpTermExtractor());

} /* namespace borealis */

#endif /* CMPTERM_H_ */

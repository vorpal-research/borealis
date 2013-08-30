/*
 * BinaryTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef BINARYTERM_H_
#define BINARYTERM_H_

#include "Protobuf/Gen/Term/BinaryTerm.pb.h"

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/BinaryTerm.proto
import "Term/Term.proto";
import "Util/ArithType.proto";

package borealis.proto;

message BinaryTerm {
    extend borealis.proto.Term {
        optional BinaryTerm ext = 17;
    }

    optional ArithType opcode = 1;
    optional Term lhv = 2;
    optional Term rhv = 3;
}

**/
class BinaryTerm: public borealis::Term {

    llvm::ArithType opcode;
    Term::Ptr lhv;
    Term::Ptr rhv;

    BinaryTerm(Type::Ptr type, llvm::ArithType opcode, Term::Ptr lhv, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            "(" + lhv->getName() + " " + llvm::arithString(opcode) + " " + rhv->getName() + ")"
        ), opcode(opcode), lhv(lhv), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(BinaryTerm);

    llvm::ArithType getOpcode() const { return opcode; }
    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        auto _lhv = tr->transform(lhv);
        auto _rhv = tr->transform(rhv);
        auto _type = getTermType(tr->FN.Type, _lhv, _rhv);
        return new Self{ _type, opcode, _lhv, _rhv };
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
        return TyF->merge(lhv->getType(), rhv->getType());
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, BinaryTerm> {
    static Dynamic<Impl> doit(
            const BinaryTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        USING_SMT_IMPL(Impl);

        auto lhvz3 = SMT<Impl>::doit(t->getLhv(), ef, ctx);
        auto rhvz3 = SMT<Impl>::doit(t->getRhv(), ef, ctx);;

        auto lhvb = lhvz3.template to<Bool>();
        auto rhvb = rhvz3.template to<Bool>();

        if(!lhvb.empty() && !rhvb.empty()) {
            auto lhv = lhvb.getUnsafe();
            auto rhv = rhvb.getUnsafe();

            switch(t->getOpcode()) {
            case llvm::ArithType::BAND:
            case llvm::ArithType::LAND: return lhv && rhv;
            case llvm::ArithType::BOR:
            case llvm::ArithType::LOR:  return lhv || rhv;
            case llvm::ArithType::XOR:  return lhv ^  rhv;
            default: BYE_BYE(Dynamic,
                             "Unsupported logic opcode: " +
                             llvm::arithString(t->getOpcode()));
            }
        }

        auto lhvbv = lhvz3.template to<DynBV>();
        auto rhvbv = rhvz3.template to<DynBV>();

        if(!lhvbv.empty() && !rhvbv.empty()) {
            auto lhv = lhvbv.getUnsafe();
            auto rhv = rhvbv.getUnsafe();

            switch(t->getOpcode()) {
            case llvm::ArithType::ADD:  return lhv +  rhv;
            case llvm::ArithType::BAND: return lhv &  rhv;
            case llvm::ArithType::BOR:  return lhv |  rhv;
            case llvm::ArithType::DIV:  return lhv /  rhv;
            case llvm::ArithType::SHL:  return lhv << rhv;
            case llvm::ArithType::MUL:  return lhv *  rhv;
            case llvm::ArithType::REM:  return lhv %  rhv;
            case llvm::ArithType::ASHR: return lhv >> rhv;
            case llvm::ArithType::SUB:  return lhv -  rhv;
            case llvm::ArithType::XOR:  return lhv ^  rhv;
            case llvm::ArithType::LSHR: return lhv.lshr(rhv);
            default: BYE_BYE(Dynamic,
                             "Unsupported bv opcode: " +
                             llvm::arithString(t->getOpcode()));
            }
        }

        BYE_BYE(Dynamic, "Unreachable!");
    }
};
#include "Util/unmacros.h"



} // namespace borealis

#endif /* BINARYTERM_H_ */

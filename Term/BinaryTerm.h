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

/** protobuf -> Term/BinaryTerm.proto
import "Term/Term.proto";
import "Util/ArithType.proto";

package borealis.proto;

message BinaryTerm {
    extend borealis.proto.Term {
        optional BinaryTerm ext = $COUNTER_TERM;
    }

    optional ArithType opcode = 1;
    optional Term lhv = 2;
    optional Term rhv = 3;
}

**/
class BinaryTerm: public borealis::Term {

    llvm::ArithType opcode;

    BinaryTerm(Type::Ptr type, llvm::ArithType opcode, Term::Ptr lhv, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(BinaryTerm);

    llvm::ArithType getOpcode() const;
    Term::Ptr getLhv() const;
    Term::Ptr getRhv() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _lhv = tr->transform(getLhv());
        auto&& _rhv = tr->transform(getRhv());
        TERM_ON_CHANGED(
            getLhv() != _lhv || getRhv() != _rhv,
            tr->FN.Term->getBinaryTerm(opcode, _lhv, _rhv)
        );
    }

    virtual bool equals(const Term* other) const override;
    virtual size_t hashCode() const override;

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

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        auto&& lhvz3 = SMT<Impl>::doit(t->getLhv(), ef, ctx);
        auto&& rhvz3 = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        Bool lhvb = lhvz3;
        Bool rhvb = rhvz3;

        if (bool(lhvb) && bool(rhvb)) {
            auto&& lhv = lhvb;
            auto&& rhv = rhvb;

            switch (t->getOpcode()) {
            case llvm::ArithType::BAND:
            case llvm::ArithType::LAND: return lhv && rhv;
            case llvm::ArithType::BOR:
            case llvm::ArithType::LOR:  return lhv || rhv;
            case llvm::ArithType::XOR:  return lhv ^  rhv;
            case llvm::ArithType::IMPLIES: return ef.implies(lhv, rhv);
            default: BYE_BYE(Dynamic,
                             "Unsupported logic opcode: " +
                             llvm::arithString(t->getOpcode()));
            }
        }

        DynBV lhvbv = lhvz3;
        DynBV rhvbv = rhvz3;

        if (bool(lhvbv) && bool(rhvbv)) {
            auto&& lhv = lhvbv;
            auto&& rhv = rhvbv;

            switch (t->getOpcode()) {
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

        BYE_BYE(Dynamic, "Unsupported BinaryTerm: " + t->getName());
    }

};
#include "Util/unmacros.h"

struct BinaryTermExtractor {

    functional_hell::matchers::storage_t<llvm::ArithType, Term::Ptr, Term::Ptr> unapply(Term::Ptr t) const {
        if (auto&& p = llvm::dyn_cast<BinaryTerm>(t)) {
            return functional_hell::matchers::make_storage(p->getOpcode(), p->getLhv(), p->getRhv());
        } else {
            return {};
        }
    }

};

static auto $BinaryTerm = functional_hell::matchers::make_pattern(BinaryTermExtractor());

} // namespace borealis

#endif /* BINARYTERM_H_ */

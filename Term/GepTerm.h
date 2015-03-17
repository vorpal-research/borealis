/*
 * GepTerm.h
 *
 *  Created on: Jun 3, 2013
 *      Author: ice-phoenix
 */

#ifndef GEPTERM_H_
#define GEPTERM_H_

#include "Term/Term.h"
#include "Term/OpaqueIntConstantTerm.h"

#include "Util/macros.h"

namespace borealis {

/** protobuf -> Term/GepTerm.proto
import "Term/Term.proto";

package borealis.proto;

message GepTerm {
    extend borealis.proto.Term {
        optional GepTerm ext = $COUNTER_TERM;
    }

    optional Term base = 1;
    repeated Term by = 2;
}

**/
class GepTerm: public borealis::Term {

    GepTerm(Type::Ptr type, Term::Ptr base, const std::vector<Term::Ptr>& shifts);

public:

    MK_COMMON_TERM_IMPL(GepTerm);

    Term::Ptr getBase() const;
    auto getShifts() const -> decltype(util::viewContainer(subterms));

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _base = tr->transform(getBase());
        auto&& _shifts = getShifts().map(
            [&](auto&& shift) { return tr->transform(shift); }
        );
        auto&& _type = type;
        TERM_ON_CHANGED(
            getBase() != _base || not util::equal(getShifts(), _shifts, ops::equals_to),
            new Self( _type, _base, _shifts.toVector() )
        );
    }

    static Type::Ptr getAggregateElement(Type::Ptr parent, Term::Ptr idx) {
        if (auto* structType = llvm::dyn_cast<type::Record>(parent)) {
            auto&& body = structType->getBody()->get();
            auto&& cIdx = llvm::dyn_cast<OpaqueIntConstantTerm>(idx);
            ASSERTC(!!cIdx);
            auto&& index = cIdx->getValue();
            ASSERTC(index >= 0);
            auto&& uIndex = (unsigned long long) index;
            ASSERTC(uIndex < body.getNumFields());
            return body.at(uIndex).getType();
        } else if (auto* arrayType = llvm::dyn_cast<type::Array>(parent)) {
            return arrayType->getElement();
        }

        BYE_BYE(Type::Ptr, "getAggregateElement on non-aggregate type: " + TypeUtils::toString(*parent));
    }

    static Type::Ptr getGepChild(Type::Ptr parent, const std::vector<Term::Ptr>& index) {
        auto&& ptrType = llvm::dyn_cast<type::Pointer>(parent);
        ASSERT(!!ptrType, "getGepChild argument is not a pointer");
        auto&& ret = ptrType->getPointed();
        for (auto&& ix : util::tail(index)) {
            ret = getAggregateElement(ret, ix);
        }
        return ret;
    }

};

template<class Impl>
struct SMTImpl<Impl, GepTerm> {
    static Dynamic<Impl> doit(
            const GepTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        auto&& base = SMT<Impl>::doit(t->getBase(), ef, ctx).template to<Pointer>();

        ASSERT(not base.empty(),
               "Encountered a GEP term with non-pointer operand");

        auto&& p = base.getUnsafe();
        auto&& shift = ef.getIntConst(0);

        auto* baseType = llvm::dyn_cast<type::Pointer>(t->getBase()->getType());
        ASSERTC(!!baseType);

        if (not t->getShifts().empty()) {
            auto&& h = util::head(t->getShifts());

            auto&& tp = baseType->getPointed();
            auto&& size = TypeUtils::getTypeSizeInElems(tp);

            auto&& by = SMT<Impl>::doit(h, ef, ctx).template to<Integer>();
            ASSERT(not by.empty(),
                   "Encountered a GEP term with incorrect shifts");

            shift = shift + by.getUnsafe() * size;

            for (auto&& s : util::tail(t->getShifts())) {

                if (llvm::isa<type::Record>(tp)) {
                    auto&& ss = llvm::dyn_cast<OpaqueIntConstantTerm>(s);
                    ASSERTC(!!ss);
                    auto&& offset = TypeUtils::getStructOffsetInElems(tp, ss->getValue());

                    shift = shift + offset;

                    tp = GepTerm::getAggregateElement(tp, s);

                } else if (llvm::isa<type::Array>(tp)) {
                    tp = GepTerm::getAggregateElement(tp, s);
                    size = TypeUtils::getTypeSizeInElems(tp);

                    auto&& by = SMT<Impl>::doit(s, ef, ctx).template to<Integer>();
                    ASSERT(not by.empty(),
                           "Encountered a GEP term with incorrect shifts");

                    shift = shift + by.getUnsafe() * size;

                } else BYE_BYE(Dynamic, "Encountered non-aggregate type in GEP: " + TypeUtils::toString(*tp));

            }
        }

        auto&& bound = ctx->getBound(p);

        return ef.if_(ef.isInvalidPtrExpr(p) || UComparable(shift).uge(bound))
                 .then_(ef.getInvalidPtr())
                 .else_(
                     (p + shift).withAxiom(
                         not ef.isInvalidPtrExpr(p + shift)
                     )
                 );
    }
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* GEPTERM_H_ */

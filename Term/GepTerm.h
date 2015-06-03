/*
 * GepTerm.h
 *
 *  Created on: Jun 3, 2013
 *      Author: ice-phoenix
 */

#ifndef GEPTERM_H_
#define GEPTERM_H_

#include "Config/config.h"
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
    optional bool triviallyInbounds = 3;
}

**/
class GepTerm: public borealis::Term {

    GepTerm(Type::Ptr type, Term::Ptr base, const std::vector<Term::Ptr>& shifts);
    GepTerm(Type::Ptr type, Term::Ptr base, const std::vector<Term::Ptr>& shifts, bool inBounds);

    bool isTriviallyInbounds_;

public:

    MK_COMMON_TERM_IMPL(GepTerm);

    Term::Ptr getBase() const;
    auto getShifts() const -> decltype(util::viewContainer(subterms));
    bool isTriviallyInbounds() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _base = tr->transform(getBase());
        auto&& _shifts = getShifts().map(
            [&](auto&& shift) { return tr->transform(shift); }
        );
        auto&& _type = type;
        TERM_ON_CHANGED(
            getBase() != _base || not util::equal(getShifts(), _shifts, ops::equals_to),
            new Self( _type, _base, _shifts.toVector(), isTriviallyInbounds_ ) // XXX: check that transformers do not invalidate isTriviallyInbounds
        );
    }

    static Type::Ptr getAggregateElement(Type::Ptr parent, Term::Ptr idx);
    static Type::Ptr getGepChild(Type::Ptr parent, const std::vector<Term::Ptr>& index);

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

        if (t->getShifts().empty() || util::viewContainer(t->getShifts())
                                           .map(llvm::dyn_caster<OpaqueIntConstantTerm>{})
                                           .all_of(LAM(I, I && I->getValue() == 0))) {
            return p;
        }

        auto&& shift = ef.getIntConst(0);

        auto* baseType = llvm::dyn_cast<type::Pointer>(t->getBase()->getType());
        ASSERTC(!!baseType);

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

        auto diff = ctx->getBound(p) - shift;
        auto zero = ef.getIntConst(0);
        auto shifted = p + shift;

        static config::BoolConfigEntry CraigColtonMode("analysis", "craig-colton-bounds");

        if(CraigColtonMode.get(false)) {
            if (t->isTriviallyInbounds()) {
                 return shifted;
            }

            return shifted.withAxiom(
                not ef.isInvalidPtrExpr(shifted)
            );
        }
        
        ctx->writeBound(shifted, diff);
        if (t->isTriviallyInbounds()) {
            return shifted;
        }

        return shifted.withAxiom(shifted > zero);
    }
};

struct GepTermExtractor {

    auto unapply(Term::Ptr t) const -> functional_hell::matchers::storage_t<Term::Ptr, decltype(std::declval<GepTerm>().getShifts())> {
        if (auto&& tt = llvm::dyn_cast<GepTerm>(t)) {
            return functional_hell::matchers::make_storage(tt->getBase(), tt->getShifts());
        } else {
            return {};
        }
    }

};

static auto $GepTerm = functional_hell::matchers::make_pattern(GepTermExtractor());

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* GEPTERM_H_ */

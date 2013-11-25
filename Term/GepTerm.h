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

    typedef Term::Ptr Shift;
    typedef std::vector<Shift> Shifts;

    Term::Ptr base;
    const Shifts shifts;

    GepTerm(Type::Ptr type, Term::Ptr base, const Shifts& shifts) :
        Term(
            class_tag(*this),
            type,
            "gep(" + base->getName() + "," +
                std::accumulate(shifts.begin(), shifts.end(), std::string{"0"},
                    [](const std::string& a, const Shift& shift) {
                        return a + "+" + shift->getName();
                    }
                ) +
            ")"
        ), base(base), shifts(shifts) {};

public:

    MK_COMMON_TERM_IMPL(GepTerm);

    Term::Ptr getBase() const { return base; }
    const Shifts& getShifts() const { return shifts; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto _base = tr->transform(base);
        auto _shifts = util::viewContainer(shifts).map(
            [&tr](const Shift& shift) { return tr->transform(shift); }
        ).toVector();
        auto _type = type;
        TERM_ON_CHANGED(
            base != _base || shifts != _shifts,
            new Self( _type, _base, _shifts )
        );
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->base == *base &&
                    std::equal(shifts.begin(), shifts.end(), that->shifts.begin(),
                        [](const Shift& a, const Shift& b) {
                            return *a == *b;
                        }
                    );
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), base, shifts);
    }

    static Type::Ptr getGepChild(Type::Ptr parent, Term::Ptr index) {
        auto intIndexTerm = llvm::dyn_cast<OpaqueIntConstantTerm>(index);
        auto realIndex = intIndexTerm ? intIndexTerm->getValue() : ~0LL;

        return TypeUtils::getGepChild(parent, (unsigned)realIndex);
    }

    static Type::Ptr getGepChild(Type::Ptr parent, const std::vector<Term::Ptr>& index) {
        auto ret = parent;
        for(auto ix : util::tail(index)) {
            ret = getGepChild(ret, ix);
        }
        return ret;
    }
};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, GepTerm> {
    static Dynamic<Impl> doit(
            const GepTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        USING_SMT_IMPL(Impl);

        auto base = SMT<Impl>::doit(t->getBase(), ef, ctx).template to<Pointer>();

        ASSERT(!base.empty(),
               "Encountered a GEP term with non-pointer operand");

        Pointer p = base.getUnsafe();
        Integer shift = ef.getIntConst(0);
        auto tp = t->getBase()->getType();
        tp = llvm::dyn_cast<type::Pointer>(tp)->getPointed();

        auto iter = 0U;
        for (const auto& s : t->getShifts()) {
            auto by = SMT<Impl>::doit(s, ef, ctx).template to<Integer>();
            auto byIx = llvm::dyn_cast<OpaqueIntConstantTerm>(s);
            auto ix = byIx? byIx->getValue() : ~0U;

            if (iter != 0U) tp = TypeUtils::getGepChild(tp, ix);
            iter++;

            auto size = TypeUtils::getTypeSizeInElems(tp);

            ASSERT(!by.empty(),
                   "Encountered a GEP term with incorrect shifts");

            shift = shift + by.getUnsafe() * size;
        }

        Integer bound = ctx->getBound(p);

        return ef.if_(ef.isInvalidPtrExpr(p) || UComparable(shift).uge(bound))
                 .then_(ef.getInvalidPtr())
                 .else_(
                     (p + shift).withAxiom(
                         !ef.isInvalidPtrExpr(p + shift)
                     )
                 );
    }
};

#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* GEPTERM_H_ */

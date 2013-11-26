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
                    util::equal(shifts, that->shifts,
                        [](const Shift& a, const Shift& b) {
                            return *a == *b;
                        }
                    );
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), base, shifts);
    }

    static Type::Ptr getAggregateElement(Type::Ptr parent, Term::Ptr idx) {
        if (auto* structType = llvm::dyn_cast<type::Record>(parent)) {
            const auto& body = structType->getBody()->get();
            auto cIdx = llvm::dyn_cast<OpaqueIntConstantTerm>(idx);
            ASSERTC(!!cIdx);
            auto index = cIdx->getValue();
            ASSERTC(index >= 0);
            auto uIndex = (unsigned long long)index;
            ASSERTC(uIndex < body.getNumFields());
            return body.at(uIndex).getType();
        } else if (auto* arrayType = llvm::dyn_cast<type::Array>(parent)) {
//            const auto& sz = arrayType->getSize();
//            if(!!sz && sz.getUnsafe() <= idx) {
//                dbgs() << "Should be detected: array index overflow"
//                       << "  Index: " << util::toString(idx)
//                       << "  Type: " << TypeUtils::toString(*arrayType)
//                       << endl;
//            }
            return arrayType->getElement();
        }

        BYE_BYE(Type::Ptr, "getAggregateElement on non-aggregate type: " + TypeUtils::toString(*parent));
    }

    static Type::Ptr getGepChild(Type::Ptr parent, const std::vector<Term::Ptr>& index) {
        auto ptrType = llvm::dyn_cast<type::Pointer>(parent);
        ASSERT(!!ptrType, "getGepChild argument is not a pointer");
        auto ret = ptrType->getPointed();
        for(auto ix : util::tail(index)) {
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

        USING_SMT_IMPL(Impl);

        auto base = SMT<Impl>::doit(t->getBase(), ef, ctx).template to<Pointer>();

        ASSERT(!base.empty(),
               "Encountered a GEP term with non-pointer operand");

        Pointer p = base.getUnsafe();
        Integer shift = ef.getIntConst(0);

        auto* baseType = llvm::dyn_cast<type::Pointer>(t->getBase()->getType());
        ASSERTC(!!baseType);

        if (!t->getShifts().empty()) {
            auto h = util::head(t->getShifts());

            auto tp = baseType->getPointed();

            auto by = SMT<Impl>::doit(h, ef, ctx).template to<Integer>();
            auto size = TypeUtils::getTypeSizeInElems(tp);
            ASSERT(!by.empty(),
                   "Encountered a GEP term with incorrect shifts");
            shift = shift + by.getUnsafe() * size;

            for (const auto& s : util::tail(t->getShifts())) {
                tp = GepTerm::getAggregateElement(tp, s);

                auto by = SMT<Impl>::doit(s, ef, ctx).template to<Integer>();
                auto size = TypeUtils::getTypeSizeInElems(tp);
                ASSERT(!by.empty(),
                       "Encountered a GEP term with incorrect shifts");
                shift = shift + by.getUnsafe() * size;
            }
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

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* GEPTERM_H_ */

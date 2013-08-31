/*
 * GepTerm.h
 *
 *  Created on: Jun 3, 2013
 *      Author: ice-phoenix
 */

#ifndef GEPTERM_H_
#define GEPTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/GepTerm.proto
import "Term/Term.proto";

package borealis.proto;

message GepTerm {
    extend borealis.proto.Term {
        optional GepTerm ext = 20;
    }

    optional Term base = 1;
    repeated Term by = 2;
    repeated Term size = 3;
}

**/
class GepTerm: public borealis::Term {

    typedef std::pair<Term::Ptr, Term::Ptr> Shift;
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
                        return a + "+" + shift.first->getName() + "*" + shift.second->getName();
                    }
                ) +
            ")"
        ), base(base), shifts(shifts) {};

public:

    MK_COMMON_TERM_IMPL(GepTerm);

    Term::Ptr getBase() const { return base; }
    const Shifts& getShifts() const { return shifts; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        Shifts _shifts;
        _shifts.reserve(shifts.size());
        std::transform(shifts.begin(), shifts.end(), std::back_inserter(_shifts),
            [&tr](const Shift& shift) {
                return std::make_pair(tr->transform(shift.first), tr->transform(shift.second));
            }
        );

        auto _base = tr->transform(base);
        auto _type = type;
        return new Self{ _type, _base, _shifts };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->base == *base &&
                    std::equal(shifts.begin(), shifts.end(), that->shifts.begin(),
                        [](const Shift& a, const Shift& b) {
                            return *a.first == *b.first && *a.second == *b.second;
                        }
                    );
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), base, shifts);
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

        for (const auto& s : t->getShifts()) {
            auto by = SMT<Impl>::doit(s.first, ef, ctx).template to<Integer>();
            auto size = SMT<Impl>::doit(s.second, ef, ctx).template to<Integer>();

            ASSERT(!by.empty() && !size.empty(),
                   "Encountered a GEP term with incorrect shifts");

            shift = shift + by.getUnsafe() * size.getUnsafe();
        }

        return ef.if_(ef.isInvalidPtrExpr(p))
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

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

class GepTerm: public borealis::Term {

    typedef std::pair<Term::Ptr, Term::Ptr> Shift;
    typedef std::vector<Shift> Shifts;

    Term::Ptr v;
    const Shifts shifts;
    const llvm::Type* type;

    GepTerm(
            Term::Ptr v,
            const Shifts& shifts,
            const llvm::Type* type
    ) : Term(
            v->hashCode() ^ std::hash<Shifts>()(shifts),
            "gep(" + v->getName() + "," +
                std::accumulate(shifts.begin(), shifts.end(), std::string{"0"},
                    [](const std::string& a, const Shift& shift) {
                        return a + "+" + shift.first->getName() + "*" + shift.second->getName();
                    }
                ) +
            ")",
            type_id(*this)
        ), v(v), shifts(shifts), type(type) {}

public:

    MK_COMMON_TERM_IMPL(GepTerm);

    Term::Ptr getBase() const { return v; }
    const Shifts& getShifts() const { return shifts; }

    template<class Sub>
    auto accept(Transformer<Sub>* t) const -> const Self* {
        Shifts transformedShifts;
        transformedShifts.reserve(shifts.size());

        std::transform(shifts.begin(), shifts.end(), std::back_inserter(transformedShifts),
            [&t](const Shift& shift) {
                return std::make_pair(t->transform(shift.first), t->transform(shift.second));
            }
        );

        return new Self{
            t->transform(v),
            transformedShifts,
            type
        };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->v == *v &&
                    std::equal(shifts.begin(), shifts.end(), that->shifts.begin(),
                        [](const Shift& e1, const Shift& e2) {
                            return *e1.first == *e2.first && *e1.second == *e2.second;
                        }
                    );
        } else return false;
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(type);
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

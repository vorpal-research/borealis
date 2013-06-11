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

    typedef GepTerm Self;

    typedef std::pair<Term::Ptr, Term::Ptr> Shift;
    typedef std::vector<Shift> Shifts;

public:

    friend class TermFactory;

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<Self>();
    }

    static bool classof(const Self* /* t */) {
        return true;
    }

    GepTerm(const Self&) = default;
    ~GepTerm();

    template<class Sub>
    auto accept(Transformer<Sub>* t) const -> const Self* {
        Shifts transformedShifts;
        transformedShifts.reserve(shifts.size());

        std::transform(shifts.begin(), shifts.end(), std::back_inserter(transformedShifts),
            [&t](const Shift& shift) {
                return std::make_pair(t->transform(shift.first), t->transform(shift.second));
            }
        );

        return new GepTerm(
            t->transform(v),
            transformedShifts,
            type
        );
    }

#include "Util/macros.h"
    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const override {
        typedef Z3ExprFactory::Dynamic Dynamic;
        typedef Z3ExprFactory::Integer Integer;
        typedef Z3ExprFactory::Pointer Pointer;

        Dynamic vv = v->toZ3(z3ef, ctx);

        ASSERT(vv.is<Pointer>(),
               "Encountered a GEP term with non-pointer operand");

        Pointer vp = vv.to<Pointer>().getUnsafe();

        Pointer shift = z3ef.getIntConst(0);
        for (const auto& s : shifts) {
            auto by = s.first->toZ3(z3ef, ctx).to<Pointer>();
            auto size = s.second->toZ3(z3ef, ctx).to<Pointer>();

            ASSERT(!by.empty() && !size.empty(),
                   "Encountered a GEP term with incorrect shifts");

            shift = shift + by.getUnsafe() * size.getUnsafe();
        }

        return z3ef.if_(z3ef.isInvalidPtrExpr(vp))
                   .then_(z3ef.getInvalidPtr())
                   .else_(
                       (vp + shift).withAxiom(
                           !z3ef.isInvalidPtrExpr(vp + shift)
                       )
                   );
    }
#include "Util/unmacros.h"

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(type);
    }

private:

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

    Term::Ptr v;
    const Shifts shifts;
    const llvm::Type* type;

};

} /* namespace borealis */

#endif /* GEPTERM_H_ */

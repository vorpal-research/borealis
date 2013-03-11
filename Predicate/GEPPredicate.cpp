/*
 * GEPPredicate.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/GEPPredicate.h"
#include "Util/macros.h"

namespace borealis {

GEPPredicate::GEPPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        std::vector< std::pair<Term::Ptr, Term::Ptr> >& shifts,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            rhv(rhv),
            shifts(shifts) {

    std::string a = "0";
    for (const auto& shift : shifts) {
        a = a + "+" + shift.first->getName() + "*" + shift.second->getName();
    }

    this->asString =
            this->lhv->getName() + "=gep(" + this->rhv->getName() + "," + a + ")";
}

logic::Bool GEPPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Dynamic Dynamic;
    typedef Z3ExprFactory::Pointer Pointer;
    typedef Z3ExprFactory::Integer Integer;

    Dynamic l = lhv->toZ3(z3ef, ctx);
    Dynamic r = rhv->toZ3(z3ef, ctx);

    ASSERT(l.is<Pointer>() && r.is<Pointer>(),
           "Encountered a GEP predicate with non-pointer operand");

    Pointer lp = l.to<Pointer>().getUnsafe();
    Pointer rp = r.to<Pointer>().getUnsafe();

    Pointer shift = z3ef.getIntConst(0);
    for (const auto& s : shifts) {
        auto by = s.first->toZ3(z3ef, ctx).to<Pointer>();
        auto size = s.second->toZ3(z3ef, ctx).to<Pointer>();

        ASSERT(!by.empty() && !size.empty(),
               "Encountered a GEP predicate with incorrect shifts");

        shift = shift + by.getUnsafe() * size.getUnsafe();
    }

    return z3ef.if_(z3ef.isInvalidPtrExpr(rp))
               .then_(lp == z3ef.getInvalidPtr())
               .else_(lp == rp+shift && !z3ef.isInvalidPtrExpr(lp));
}

bool GEPPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const GEPPredicate* o = llvm::dyn_cast<GEPPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv &&
                std::equal(this->shifts.begin(), this->shifts.end(), o->shifts.begin(),
                    [](const std::pair<Term::Ptr, Term::Ptr>& a, const std::pair<Term::Ptr, Term::Ptr>& b) {
                        return *a.first == *b.first && *a.second == *b.second;
                    }
                );
    } else {
        return false;
    }
}

size_t GEPPredicate::hashCode() const {
    return util::hash::hasher<3, 17>()(type, lhv, rhv, shifts);
}

} /* namespace borealis */

#include "Util/unmacros.h"

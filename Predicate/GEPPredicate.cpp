/*
 * GEPPredicate.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/GEPPredicate.h"
#include "Term/ConstTerm.h"
#include "Term/ValueTerm.h"
#include "Util/macros.h"

namespace borealis {

GEPPredicate::GEPPredicate(
        PredicateType type,
        Term::Ptr lhv,
        Term::Ptr rhv,
        std::vector< std::pair< Term::Ptr, Term::Ptr > >& shifts) :
            GEPPredicate(lhv, rhv, shifts, nullptr, type) {}

GEPPredicate::GEPPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        std::vector< std::pair< Term::Ptr, Term::Ptr > >& shifts,
        SlotTracker* /* st */,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            rhv(rhv) {

    std::string a = "0";
    for (const auto& shift : shifts) {
        a = a + "+" + shift.first->getName() + "*" + shift.second->getName();
    }

    this->asString =
            this->lhv->getName() + "=gep(" + this->rhv->getName() + "," + a + ")";
}

logic::Bool GEPPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Dynamic Dynamic;
    typedef Z3ExprFactory::Pointer Pointer;
    typedef Z3ExprFactory::Integer Integer;

    Dynamic l = z3ef.getExprForTerm(*lhv);
    Dynamic r = z3ef.getExprForTerm(*rhv);

    if (!l.is<Pointer>() || !r.is<Pointer>()) {
        BYE_BYE(logic::Bool, "Encountered a GEP predicate with non-pointer operand");
    }

    Pointer lp = l.to<Pointer>().getUnsafe();
    Pointer rp = r.to<Pointer>().getUnsafe();

    const size_t ptrsize = Pointer::bitsize;

    Integer shift = z3ef.getIntConst(0, ptrsize);
    for (const auto& s : shifts) {
        Pointer by = z3ef.getExprForTerm(*s.first, ptrsize).to<Pointer>().getUnsafe();
        Pointer size = z3ef.getExprForTerm(*s.second, ptrsize).to<Pointer>().getUnsafe();

        shift = shift + by * size;
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
                *this->rhv == *o->rhv;
        // FIXME: Compare this->shifts and other->shifts
    } else {
        return false;
    }
}

size_t GEPPredicate::hashCode() const {
    // FIXME: Hash this->shifts as well
    return util::hash::hasher<3, 17>()(lhv, rhv);
}

} /* namespace borealis */

#include "Util/unmacros.h"

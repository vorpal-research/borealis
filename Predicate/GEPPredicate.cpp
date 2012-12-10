/*
 * GEPPredicate.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: ice-phoenix
 */

#include "GEPPredicate.h"

#include "Term/ConstTerm.h"
#include "Term/ValueTerm.h"

namespace borealis {

GEPPredicate::GEPPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        std::vector< std::pair< Term::Ptr, Term::Ptr > >& shifts) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            rhv(std::move(rhv)),
            shifts(std::move(shifts)) {

    std::string a = "0";
    for (const auto& shift : shifts) {
        a = a + "+" + shift.first->getName() + "*" + shift.second->getName();
    }

    this->asString =
            this->lhv->getName() + "=gep(" + this->rhv->getName() + "," + a + ")";
}

GEPPredicate::GEPPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        std::vector< std::pair<llvm::Value*, uint64_t> >& shifts,
        SlotTracker* st) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            rhv(std::move(rhv)) {

    std::string a = "0";
    for (const auto& shift : shifts) {
        ValueTerm* by = new ValueTerm(shift.first, st);
        ConstTerm* size = new ConstTerm(llvm::ValueType::INT_CONST, util::toString(shift.second));
        this->shifts.push_back(
                std::make_pair(Term::Ptr(by), Term::Ptr(size))
        );

        a = a + "+" + by->getName() + "*" + size->getName();
    }

    this->asString =
            this->lhv->getName() + "=gep(" + this->rhv->getName() + "," + a + ")";
}

Predicate::Key GEPPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv->getId());
}

z3::expr GEPPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    z3::expr l = z3ef.getExprForTerm(*lhv);
    z3::expr r = z3ef.getExprForTerm(*rhv);

    size_t ptrsize = z3ef.getPtrSort().bv_size();

    z3::expr shift = z3ef.getIntConst(0, ptrsize);
    for (const auto& s : shifts) {
        z3::expr by = z3ef.getExprForTerm(*s.first, ptrsize);
        z3::expr size = z3ef.getExprForTerm(*s.second, ptrsize);

        shift = shift + by * size;
    }

    return l == z3ef.if_(z3ef.isInvalidPtrExpr(r))
                    .then_(z3ef.getInvalidPtr())
                    .else_(r+shift);
}

bool GEPPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const GEPPredicate* o = llvm::dyn_cast<GEPPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv;
        // FIXME: akhin Compare this->shifts and other->shifts
    } else {
        return false;
    }
}

size_t GEPPredicate::hashCode() const {
    size_t hash = 3;
    hash = 17 * hash + lhv->hashCode();
    hash = 17 * hash + rhv->hashCode();
    // FIXME: akhin Hash this->shifts as well
    return hash;
}

} /* namespace borealis */

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

logic::Bool GEPPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Pointer Pointer;
    typedef Z3ExprFactory::Dynamic Dynamic;
    typedef Z3ExprFactory::Integer Integer;

    Dynamic l = z3ef.getExprForTerm(*lhv);
    Dynamic r = z3ef.getExprForTerm(*rhv);

    if (!l.is<Pointer>() || !r.is<Pointer>()) {
        return util::sayonara<logic::Bool>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Encountered a GEP predicate with non-pointer operand");
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

    return lp == z3ef.if_(z3ef.isInvalidPtrExpr(rp))
                     .then_(z3ef.getInvalidPtr())
                     .else_(rp+shift);
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

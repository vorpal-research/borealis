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
        std::vector< std::pair<llvm::Value*, uint64_t> > shifts,
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

Predicate::Dependee GEPPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv->getId());
}

Predicate::DependeeSet GEPPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    // TODO akhin
    return res;
}

z3::expr GEPPredicate::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    expr l = z3ef.getExprForTerm(*lhv);
    expr r = z3ef.getExprForTerm(*rhv);

    func_decl gep = z3ef.getGEPFunction();

    expr shift = z3ef.getIntConst(0);
    for (const auto& s : shifts) {
        expr by = z3ef.getExprForTerm(*s.first);
        expr size = z3ef.getExprForTerm(*s.second);;
        shift = shift + by * size;
    }

    return l == gep(r, shift);
}

} /* namespace borealis */

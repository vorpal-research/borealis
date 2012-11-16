/*
 * ICmpPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "ICmpPredicate.h"

#include "ValueTerm.h"

namespace borealis {

using llvm::ConditionType;
using llvm::conditionString;
using llvm::conditionType;

using util::sayonara;

ICmpPredicate::ICmpPredicate(
        const llvm::Value* lhv,
        const llvm::Value* op1,
        const llvm::Value* op2,
        const int cond,
        SlotTracker* st) :
				        lhv(new ValueTerm(lhv, st)),
				        op1(new ValueTerm(op1, st)),
				        op2(new ValueTerm(op2, st)),
				        cond(cond),
				        _cond(conditionString(cond)) {
    this->asString =
            this->lhv->getName() + "=" + this->op1->getName() + _cond + this->op2->getName();
}

Predicate::Key ICmpPredicate::getKey() const {
    return std::make_pair(borealis::type_id(*this), lhv->getId());
}

Predicate::Dependee ICmpPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv->getId());
}

Predicate::DependeeSet ICmpPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, op1->getId()));
    res.insert(std::make_pair(DependeeType::VALUE, op2->getId()));
    return res;
}

z3::expr ICmpPredicate::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    expr l = z3ef.getExprForTerm(*lhv);
    expr o1 = z3ef.getExprForTerm(*op1);
    expr o2 = z3ef.getExprForTerm(*op2);

    ConditionType ct = conditionType(cond);
    switch(ct) {
    case ConditionType::EQ: return l == (o1 == o2);
    case ConditionType::NEQ: return l == (o1 != o2);
    case ConditionType::LT: return l == (o1 < o2);
    case ConditionType::LTE: return l == (o1 <= o2);
    case ConditionType::GT: return l == (o1 > o2);
    case ConditionType::GTE: return l == (o1 >= o2);
    case ConditionType::TRUE: return l == z3ef.getBoolConst(true);
    case ConditionType::FALSE: return l == z3ef.getBoolConst(false);
    case ConditionType::UNKNOWN: return sayonara<z3::expr>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
            "Unknown condition type in Z3 conversion");
    }
}

} /* namespace borealis */

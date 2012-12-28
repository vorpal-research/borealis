/*
 * ArithPredicate.cpp
 *
 *  Created on: Dec 28, 2012
 *      Author: ice-phoenix
 */

#include "ArithPredicate.h"

#include "Util/macros.h"

namespace borealis {

ArithPredicate::ArithPredicate(
        Term::Ptr lhv,
        Term::Ptr op1,
        Term::Ptr op2,
        llvm::ArithType opCode) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            op1(std::move(op1)),
            op2(std::move(op2)),
            opCode(opCode),
            _opCode(llvm::arithString(opCode)) {
    this->asString = lhv->getName() + "=" +
            op1->getName() +
            _opCode +
            op2->getName();

}

ArithPredicate::ArithPredicate(
        Term::Ptr lhv,
        Term::Ptr op1,
        Term::Ptr op2,
        llvm::ArithType opCode,
        SlotTracker* /* st */) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            op1(std::move(op1)),
            op2(std::move(op2)),
            opCode(opCode),
            _opCode(llvm::arithString(opCode)) {
    this->asString = lhv->getName() + "=" +
            op1->getName() +
            _opCode +
            op2->getName();
}

Predicate::Key ArithPredicate::getKey() const {
    return std::make_pair(borealis::type_id(*this), lhv->getId());
}

logic::Bool ArithPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Integer Integer;

    auto le = z3ef.getExprForTerm(*lhv).to<Integer>();

    auto oper1 = z3ef.getExprForTerm(*op1).to<Integer>();
    auto oper2 = z3ef.getExprForTerm(*op2).to<Integer>();

    if (le.empty() || oper1.empty() || oper2.empty()) {
        BYE_BYE(logic::Bool, "Encountered arithmetic operation with non-Integer operands");
    }

    auto& l = le.getUnsafe();
    auto& o1 = oper1.getUnsafe();
    auto& o2 = oper2.getUnsafe();

    switch (opCode) {
    case llvm::ArithType::ADD: return l == o1 + o2;
    case llvm::ArithType::SUB: return l == o1 - o2;
    case llvm::ArithType::MUL: return l == o1 * o2;
    case llvm::ArithType::DIV: return l == o1 / o2;
    // FIXME: add support for REM
    default: BYE_BYE(logic::Bool, "Encountered unsupported arithmetic operation: " + _opCode);
    }
}

bool ArithPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const ArithPredicate* o = llvm::dyn_cast<ArithPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->op1 == *o->op1 &&
                *this->op2 == *o->op2 &&
                this->opCode == o->opCode;
    } else {
        return false;
    }
}

size_t ArithPredicate::hashCode() const {
    size_t hash = 3;
    hash = 17 * hash + lhv->hashCode();
    hash = 17 * hash + op1->hashCode();
    hash = 17 * hash + op2->hashCode();
    hash = 17 * hash + util::enums::asInteger(opCode);
    return hash;
}

} /* namespace borealis */

#include "Util/unmacros.h"

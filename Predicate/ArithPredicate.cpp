/*
 * ArithPredicate.cpp
 *
 *  Created on: Dec 28, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/ArithPredicate.h"
#include "Util/macros.h"

namespace borealis {

ArithPredicate::ArithPredicate(
        PredicateType type,
        Term::Ptr lhv,
        Term::Ptr op1,
        Term::Ptr op2,
        llvm::ArithType opCode) :
            ArithPredicate(lhv, op1, op2, opCode, nullptr, type) {}

ArithPredicate::ArithPredicate(
        Term::Ptr lhv,
        Term::Ptr op1,
        Term::Ptr op2,
        llvm::ArithType opCode,
        SlotTracker* /* st */,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            op1(op1),
            op2(op2),
            opCode(opCode),
            _opCode(llvm::arithString(opCode)) {

    this->asString = this->lhv->getName() + "=" +
            this->op1->getName() +
            _opCode +
            this->op2->getName();
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
    // FIXME: add support for other ArithType
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
    return util::hash::hasher<3, 17>()(lhv, op1, op2, opCode);
}

} /* namespace borealis */

#include "Util/unmacros.h"

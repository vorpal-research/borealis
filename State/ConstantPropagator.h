/*
 * ConstantPropagator.h
 *
 *  Created on: Mar 21, 2013
 *      Author: snowball
 */

#ifndef CONSTANTPROPAGATOR_H_
#define CONSTANTPROPAGATOR_H_

#include "State/Transformer.hpp"
#include "Util/macros.h"

namespace borealis {

class ConstantPropagator: public borealis::Transformer<ConstantPropagator> {
private:
    static constexpr const double EPSILON = 0.00001;
    TermFactory* TF_;

public:
    ConstantPropagator(TermFactory* TF) : TF_(TF) {}

    Term::Ptr transformBinaryTerm(BinaryTermPtr);
    Term::Ptr transformUnaryTerm(UnaryTermPtr);
    Term::Ptr transformCmpTerm(CmpTermPtr);

private:
    template<class T>
    Term::Ptr transformUnifiedBinaryTerm(T op, Term::Ptr leftTerm, Term::Ptr rightTerm) {
        using namespace llvm;

        auto lhv = transform(leftTerm);
        auto rhv = transform(rightTerm);

        if (auto tleft = dyn_cast<OpaqueBoolConstantTerm>(lhv)) {
            if (auto tright = dyn_cast<OpaqueBoolConstantTerm>(rhv)) {
                return propagateTerm(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<OpaqueIntConstantTerm>(rhv)) {
                return propagateTerm(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<OpaqueFloatingConstantTerm>(rhv)) {
                return propagateTermWithDouble(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<ConstTerm>(rhv)) {
                if (auto t = dyn_cast<ConstantInt>(tright->getConstant())) {
                    return propagateTerm(op, tleft->getValue(), (long long) t->getSExtValue());
                } else if (auto t = dyn_cast<ConstantFP>(tright->getConstant())) {
                    return propagateTermWithDouble(op, tleft->getValue(), t->getValueAPF().convertToDouble());
                }
            }
        } else if (auto tleft = dyn_cast<OpaqueIntConstantTerm>(lhv)) {
            if (auto tright = dyn_cast<OpaqueBoolConstantTerm>(rhv)) {
                return propagateTerm(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<OpaqueIntConstantTerm>(rhv)) {
                return propagateTerm(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<OpaqueFloatingConstantTerm>(rhv)) {
                return propagateTermWithDouble(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<ConstTerm>(rhv)) {
                if (auto t = dyn_cast<ConstantInt>(tright->getConstant())) {
                    return propagateTerm(op, tleft->getValue(), (long long) t->getSExtValue());
                } else if (auto t = dyn_cast<ConstantFP>(tright->getConstant())) {
                    return propagateTermWithDouble(op, tleft->getValue(), t->getValueAPF().convertToDouble());
                }
            }
        } else if (auto tleft = dyn_cast<OpaqueFloatingConstantTerm>(lhv)) {
            if (auto tright = dyn_cast<OpaqueBoolConstantTerm>(rhv)) {
                return propagateTermWithDouble(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<OpaqueIntConstantTerm>(rhv)) {
                return propagateTermWithDouble(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<OpaqueFloatingConstantTerm>(rhv)) {
                return propagateTermWithDouble(op, tleft->getValue(), tright->getValue());
            } else if (auto tright = dyn_cast<ConstTerm>(rhv)) {
                if (auto t = dyn_cast<ConstantInt>(tright->getConstant())) {
                    return propagateTermWithDouble(op, tleft->getValue(), (long long) t->getSExtValue());
                } else if (auto t = dyn_cast<ConstantFP>(tright->getConstant())) {
                    return propagateTermWithDouble(op, tleft->getValue(), t->getValueAPF().convertToDouble());
                }
            }
        } else if (auto tleft = dyn_cast<ConstTerm>(lhv)) {
            if (auto t = dyn_cast<ConstantInt>(tleft->getConstant())) {
                if (auto tright = dyn_cast<OpaqueBoolConstantTerm>(rhv)) {
                    return propagateTerm(op, (long long) t->getSExtValue(), tright->getValue());
                } else if (auto tright = dyn_cast<OpaqueIntConstantTerm>(rhv)) {
                    return propagateTerm(op, (long long) t->getSExtValue(), tright->getValue());
                } else if (auto tright = dyn_cast<OpaqueFloatingConstantTerm>(rhv)) {
                    return propagateTermWithDouble(op, (long long) t->getSExtValue(), tright->getValue());
                } else if (auto tright = dyn_cast<ConstTerm>(rhv)) {
                    if (auto t2 = dyn_cast<ConstantInt>(tright->getConstant())) {
                        return propagateTerm(op, (long long) t->getSExtValue(),
                                                   (long long) t2->getSExtValue());
                    } else if (auto t2 = dyn_cast<ConstantFP>(tright->getConstant())) {
                        return propagateTermWithDouble(op, (long long) t->getSExtValue(),
                                                             t2->getValueAPF().convertToDouble());
                    }
                }
            } else if (auto t = dyn_cast<ConstantFP>(tleft->getConstant())) {
                if (auto tright = dyn_cast<OpaqueBoolConstantTerm>(rhv)) {
                    return propagateTermWithDouble(op, t->getValueAPF().convertToDouble(), tright->getValue());
                } else if (auto tright = dyn_cast<OpaqueIntConstantTerm>(rhv)) {
                    return propagateTermWithDouble(op, t->getValueAPF().convertToDouble(), tright->getValue());
                } else if (auto tright = dyn_cast<OpaqueFloatingConstantTerm>(rhv)) {
                    return propagateTermWithDouble(op, t->getValueAPF().convertToDouble(), tright->getValue());
                } else if (auto tright = dyn_cast<ConstTerm>(rhv)) {
                    if (auto t2 = dyn_cast<ConstantInt>(tright->getConstant())) {
                        return propagateTermWithDouble(op, t->getValueAPF().convertToDouble(),
                                                            (long long) t2->getSExtValue());
                    } else if (auto t2 = dyn_cast<ConstantFP>(tright->getConstant())) {
                        return propagateTermWithDouble(op, t->getValueAPF().convertToDouble(),
                                                            t2->getValueAPF().convertToDouble());
                    }
                }
            }
        }
        return nullptr;
    }

    template<class T>
    T propagate(llvm::UnaryArithType opcode, T operand) {
        switch (opcode) {
        case llvm::UnaryArithType::NOT:
            return !operand;
        case llvm::UnaryArithType::BNOT:
            return ~operand;
        case llvm::UnaryArithType::NEG:
            return -operand;
        }
    }

    template<class Lhv, class Rhv>
    long long propagate(llvm::ArithType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ArithType::ADD:
            return lhv + rhv;
        case llvm::ArithType::BAND:
            return lhv & rhv;
        case llvm::ArithType::BOR:
            return lhv | rhv;
        case llvm::ArithType::DIV:
            return lhv / rhv;
        case llvm::ArithType::LAND:
            return lhv && rhv;
        case llvm::ArithType::LOR:
            return lhv || rhv;
        case llvm::ArithType::LSH:
            return lhv << rhv;
        case llvm::ArithType::MUL:
            return lhv * rhv;
        case llvm::ArithType::REM:
            return lhv % rhv;
        case llvm::ArithType::RSH:
            return lhv >> rhv;
        case llvm::ArithType::SUB:
            return lhv - rhv;
        case llvm::ArithType::XOR:
            return lhv ^ rhv;
        }
    }

    template<class Lhv, class Rhv>
    bool propagate(llvm::ConditionType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ConditionType::EQ:
            return lhv == rhv;
        case llvm::ConditionType::GT:
            return lhv > rhv;
        case llvm::ConditionType::GTE:
            return lhv >= rhv;
        case llvm::ConditionType::LT:
            return lhv < rhv;
        case llvm::ConditionType::LTE:
            return lhv <= rhv;
        case llvm::ConditionType::NEQ:
            return lhv != rhv;
        case llvm::ConditionType::TRUE:
            return true;
        case llvm::ConditionType::FALSE:
            return false;
        case llvm::ConditionType::UNKNOWN:
            BYE_BYE(bool, "Unknown comparison operation");
        }
    }

    template<class Lhv, class Rhv>
    double propagateDouble(llvm::ArithType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ArithType::ADD:
            return lhv + rhv;
        case llvm::ArithType::DIV:
            return lhv / rhv;
        case llvm::ArithType::LAND:
            return lhv && rhv;
        case llvm::ArithType::LOR:
            return lhv || rhv;
        case llvm::ArithType::MUL:
            return lhv * rhv;
        case llvm::ArithType::SUB:
            return lhv - rhv;
        default:
            BYE_BYE(double, "Invalid binary operation with floating point value");
        }
    }

    template<class Lhv, class Rhv>
    bool propagateDouble(llvm::ConditionType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ConditionType::EQ:
            return std::fabs(lhv - rhv) < EPSILON;
        case llvm::ConditionType::GT:
            return lhv > rhv;
        case llvm::ConditionType::GTE:
            return lhv > rhv || std::fabs(lhv - rhv) < EPSILON;
        case llvm::ConditionType::LT:
            return lhv < rhv;
        case llvm::ConditionType::LTE:
            return lhv < rhv || std::fabs(lhv - rhv) < EPSILON;
        case llvm::ConditionType::NEQ:
            return std::fabs(lhv - rhv) > EPSILON;
        case llvm::ConditionType::TRUE:
            return true;
        case llvm::ConditionType::FALSE:
            return false;
        case llvm::ConditionType::UNKNOWN:
            BYE_BYE(bool, "Unknown comparison operation");
        }
    }

    template<class T>
    Term::Ptr propagateTerm(llvm::UnaryArithType opcode, T operand) {
        return TF_->getOpaqueConstantTerm(propagate(opcode, operand));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ArithType opcode, Lhv lhv, Rhv rhv) {
        return TF_->getOpaqueConstantTerm(propagate(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ConditionType opcode, Lhv lhv, Rhv rhv) {
        return TF_->getOpaqueConstantTerm(propagate(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTermWithDouble(llvm::ArithType opcode, Lhv lhv, Rhv rhv) {
        return TF_->getOpaqueConstantTerm(propagateDouble(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTermWithDouble(llvm::ConditionType opcode, Lhv lhv, Rhv rhv) {
        return TF_->getOpaqueConstantTerm(propagateDouble(opcode, lhv, rhv));
    }
};

template<>
inline double ConstantPropagator::propagate(llvm::UnaryArithType opcode, double operand) {
    switch (opcode) {
    case llvm::UnaryArithType::NOT:
        return !operand;
    case llvm::UnaryArithType::NEG:
        return -operand;
    case llvm::UnaryArithType::BNOT:
        BYE_BYE(double, "Invalid unary operation with floating point value");
    }
}
} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CONSTANTPROPAGATOR_H_ */

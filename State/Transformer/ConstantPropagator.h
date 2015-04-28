/*
 * ConstantPropagator.h
 *
 *  Created on: Mar 21, 2013
 *      Author: snowball
 */

#ifndef CONSTANTPROPAGATOR_H_
#define CONSTANTPROPAGATOR_H_

#include <limits>
#include <type_traits>

#include "State/Transformer/Transformer.hpp"
#include "Util/cast.hpp"

#include "Util/macros.h"

namespace borealis {

class ConstantPropagator : public borealis::Transformer<ConstantPropagator> {

    typedef borealis::Transformer<ConstantPropagator> Base;

    static constexpr double EPSILON = 4 * std::numeric_limits<double>::epsilon();

public:

    ConstantPropagator(FactoryNest FN) : Base(FN) {}

    Term::Ptr transformBinaryTerm(BinaryTermPtr);
    Term::Ptr transformUnaryTerm(UnaryTermPtr);
    Term::Ptr transformCmpTerm(CmpTermPtr);

private:

    template<class T>
    Term::Ptr transformUnifiedBinaryTerm(T op, Term::Ptr leftTerm, Term::Ptr rightTerm) {
        using namespace llvm;
        using borealis::util::match_pair;

        typedef OpaqueBoolConstantTerm Bool;
        typedef OpaqueIntConstantTerm Int;
        typedef OpaqueFloatingConstantTerm Float;

        auto lhv = leftTerm;
        auto rhv = rightTerm;

#define PROPAGATE(A, B) \
    if (auto matched = match_pair<A, B>(lhv, rhv)) { \
        return propagateTerm(op, matched->first->getValue(), matched->second->getValue()); \
    }

        PROPAGATE(Bool, Bool);
        PROPAGATE(Bool, Int);
        PROPAGATE(Bool, Float);
        PROPAGATE(Int, Bool);
        PROPAGATE(Int, Int);
        PROPAGATE(Int, Float);
        PROPAGATE(Float, Bool);
        PROPAGATE(Float, Int);
        PROPAGATE(Float, Float);

#undef PROPAGATE

        return nullptr;
    }

    template<class T>
    T propagate(llvm::UnaryArithType opcode, T operand) {
        switch (opcode) {
        case llvm::UnaryArithType::NOT:  return !operand;
        case llvm::UnaryArithType::BNOT: return ~operand;
        case llvm::UnaryArithType::NEG:  return -operand;
        }
    }

    template<class Lhv, class Rhv>
    long long propagate(llvm::ArithType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ArithType::ADD:  return lhv +  rhv;
        case llvm::ArithType::BAND: return lhv &  rhv;
        case llvm::ArithType::BOR:  return lhv |  rhv;
        case llvm::ArithType::DIV:  return lhv /  rhv;
        case llvm::ArithType::LAND: return lhv && rhv;
        case llvm::ArithType::LOR:  return lhv || rhv;
        case llvm::ArithType::SHL:  return lhv << rhv;
        case llvm::ArithType::MUL:  return lhv *  rhv;
        case llvm::ArithType::REM:  return lhv %  rhv;
        case llvm::ArithType::ASHR: return lhv >> rhv;
        case llvm::ArithType::LSHR: return lhv >> rhv; // FIXME: Should be allowed only for unsigned types
        case llvm::ArithType::SUB:  return lhv -  rhv;
        case llvm::ArithType::XOR:  return lhv ^  rhv;
        case llvm::ArithType::IMPLIES: return lhv? rhv : 1;
        }
    }

    template<class Lhv, class Rhv>
    bool propagate(llvm::ConditionType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ConditionType::EQ:    return lhv == rhv;
        case llvm::ConditionType::NEQ:   return lhv != rhv;

        case llvm::ConditionType::GT:
        case llvm::ConditionType::UGT:   return lhv >  rhv;
        case llvm::ConditionType::GE:
        case llvm::ConditionType::UGE:   return lhv >= rhv;
        case llvm::ConditionType::LT:
        case llvm::ConditionType::ULT:   return lhv <  rhv;
        case llvm::ConditionType::LE:
        case llvm::ConditionType::ULE:   return lhv <= rhv;

        case llvm::ConditionType::TRUE:  return true;
        case llvm::ConditionType::FALSE: return false;
        case llvm::ConditionType::UNKNOWN:
            BYE_BYE(bool, "Unknown comparison operation");
        }
    }

    template<class Lhv, class Rhv>
    double propagateDouble(llvm::ArithType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ArithType::ADD:  return lhv +  rhv;
        case llvm::ArithType::DIV:  return lhv /  rhv;
        case llvm::ArithType::LAND: return lhv && rhv;
        case llvm::ArithType::LOR:  return lhv || rhv;
        case llvm::ArithType::MUL:  return lhv *  rhv;
        case llvm::ArithType::SUB:  return lhv -  rhv;
        default:
            BYE_BYE(double, "Invalid binary operation with floating point value");
        }
    }

    template<class Lhv, class Rhv>
    bool propagateDouble(llvm::ConditionType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ConditionType::EQ:
            return std::fabs(lhv - rhv) < EPSILON;
        case llvm::ConditionType::NEQ:
            return std::fabs(lhv - rhv) >= EPSILON;

        case llvm::ConditionType::GT:
        case llvm::ConditionType::UGT:
            return lhv > rhv;
        case llvm::ConditionType::GE:
        case llvm::ConditionType::UGE:
            return lhv > rhv || std::fabs(lhv - rhv) < EPSILON;
        case llvm::ConditionType::LT:
        case llvm::ConditionType::ULT:
            return lhv < rhv;
        case llvm::ConditionType::LE:
        case llvm::ConditionType::ULE:
            return lhv < rhv || std::fabs(lhv - rhv) < EPSILON;

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
        return FN.Term->getOpaqueConstantTerm(propagate(opcode, operand));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ArithType opcode, Lhv lhv, Rhv rhv,
            GUARDED(void*, !std::is_floating_point<Lhv>::value && !std::is_floating_point<Rhv>::value) = nullptr) {
        return FN.Term->getOpaqueConstantTerm(propagate(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ConditionType opcode, Lhv lhv, Rhv rhv,
            GUARDED(void*, !std::is_floating_point<Lhv>::value && !std::is_floating_point<Rhv>::value) = nullptr) {
        return FN.Term->getOpaqueConstantTerm(propagate(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ArithType opcode, Lhv lhv, Rhv rhv,
            GUARDED(void*, std::is_floating_point<Lhv>::value || std::is_floating_point<Rhv>::value) = nullptr) {
        return FN.Term->getOpaqueConstantTerm(propagateDouble(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ConditionType opcode, Lhv lhv, Rhv rhv,
            GUARDED(void*, std::is_floating_point<Lhv>::value || std::is_floating_point<Rhv>::value) = nullptr) {
        return FN.Term->getOpaqueConstantTerm(propagateDouble(opcode, lhv, rhv));
    }
};

template<>
inline double ConstantPropagator::propagate(llvm::UnaryArithType opcode, double operand) {
    switch (opcode) {
    case llvm::UnaryArithType::NOT: return !operand;
    case llvm::UnaryArithType::NEG: return -operand;
    case llvm::UnaryArithType::BNOT:
        BYE_BYE(double, "Invalid unary operation with floating point value");
    }
}

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CONSTANTPROPAGATOR_H_ */

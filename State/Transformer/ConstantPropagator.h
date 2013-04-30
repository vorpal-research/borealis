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
private:
    static constexpr double EPSILON = 4 * std::numeric_limits<double>::epsilon();
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
        using borealis::util::match_pair;

        typedef OpaqueBoolConstantTerm Bool;
        typedef OpaqueIntConstantTerm Int;
        typedef OpaqueFloatingConstantTerm Float;
        typedef ConstTerm Const;

        auto lhv = leftTerm;
        auto rhv = rightTerm;

#define PROPAGATE(A, B) \
    if (auto matched = match_pair<A, B>(lhv, rhv)) { \
        return propagateTerm(op, matched->first->getValue(), matched->second->getValue()); \
    }
#define PROPAGATE_WITH_CONSTANT(A) \
    if (auto matched = match_pair<A, Const>(lhv, rhv)) { \
        if (auto constInt = dyn_cast<ConstantInt>(matched->second->getConstant())) { \
            return propagateTerm(op, matched->first->getValue(), static_cast<long long>(constInt->getSExtValue())); \
        } else if (auto constFP = dyn_cast<ConstantFP>(matched->second->getConstant())) { \
            return propagateTerm(op, matched->first->getValue(), constFP->getValueAPF().convertToDouble()); \
        } \
    }
#define PROPAGATE_CONSTANT_WITH(A) \
    if (auto matched = match_pair<Const, A>(lhv, rhv)) { \
        if (auto constInt = dyn_cast<ConstantInt>(matched->first->getConstant())) { \
            return propagateTerm(op, static_cast<long long>(constInt->getSExtValue()), matched->second->getValue()); \
        } else if (auto constFP = dyn_cast<ConstantFP>(matched->first->getConstant())) { \
            return propagateTerm(op, constFP->getValueAPF().convertToDouble(), matched->second->getValue()); \
        } \
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

        PROPAGATE_WITH_CONSTANT(Bool);
        PROPAGATE_WITH_CONSTANT(Int);
        PROPAGATE_WITH_CONSTANT(Float);
        PROPAGATE_CONSTANT_WITH(Bool);
        PROPAGATE_CONSTANT_WITH(Int);
        PROPAGATE_CONSTANT_WITH(Float);

        // Very special corner case
        if (auto m = match_pair<Const, Const>(lhv, rhv)) {
            if (auto mm = match_pair<ConstantInt, ConstantInt>(m->first->getConstant(), m->second->getConstant())) {
                return propagateTerm(op,
                        static_cast<long long>(mm->first->getSExtValue()),
                        static_cast<long long>(mm->second->getSExtValue())
                );
            } else if (auto mm = match_pair<ConstantInt, ConstantFP>(m->first->getConstant(), m->second->getConstant())) {
                return propagateTerm(op,
                        static_cast<long long>(mm->first->getSExtValue()),
                        mm->second->getValueAPF().convertToDouble()
                );
            } else if (auto mm = match_pair<ConstantFP, ConstantInt>(m->first->getConstant(), m->second->getConstant())) {
                return propagateTerm(op,
                        mm->first->getValueAPF().convertToDouble(),
                        static_cast<long long>(mm->second->getSExtValue())
                );
            } else if (auto mm = match_pair<ConstantFP, ConstantFP>(m->first->getConstant(), m->second->getConstant())) {
                return propagateTerm(op,
                        mm->first->getValueAPF().convertToDouble(),
                        mm->second->getValueAPF().convertToDouble()
                );
            }
        }

#undef PROPAGATE_CONSTANT_WITH
#undef PROPAGATE_WITH_CONSTANT
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
        }
    }

    template<class Lhv, class Rhv>
    bool propagate(llvm::ConditionType opcode, Lhv lhv, Rhv rhv) {
        switch (opcode) {
        case llvm::ConditionType::EQ:    return lhv == rhv;
        case llvm::ConditionType::GT:    return lhv >  rhv;
        case llvm::ConditionType::GTE:   return lhv >= rhv;
        case llvm::ConditionType::LT:    return lhv <  rhv;
        case llvm::ConditionType::LTE:   return lhv <= rhv;
        case llvm::ConditionType::NEQ:   return lhv != rhv;
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
    Term::Ptr propagateTerm(llvm::ArithType opcode, Lhv lhv, Rhv rhv,
            typename std::enable_if<
                !std::is_floating_point<Lhv>::value && !std::is_floating_point<Rhv>::value
            >::type* = nullptr) {
        return TF_->getOpaqueConstantTerm(propagate(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ConditionType opcode, Lhv lhv, Rhv rhv,
            typename std::enable_if<
                !std::is_floating_point<Lhv>::value && !std::is_floating_point<Rhv>::value
            >::type* = nullptr) {
        return TF_->getOpaqueConstantTerm(propagate(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ArithType opcode, Lhv lhv, Rhv rhv,
            typename std::enable_if<
                std::is_floating_point<Lhv>::value || std::is_floating_point<Rhv>::value
            >::type* = nullptr) {
        return TF_->getOpaqueConstantTerm(propagateDouble(opcode, lhv, rhv));
    }

    template<class Lhv, class Rhv>
    Term::Ptr propagateTerm(llvm::ConditionType opcode, Lhv lhv, Rhv rhv,
            typename std::enable_if<
                std::is_floating_point<Lhv>::value || std::is_floating_point<Rhv>::value
            >::type* = nullptr) {
        return TF_->getOpaqueConstantTerm(propagateDouble(opcode, lhv, rhv));
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

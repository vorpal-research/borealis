/*
 * ConstantPropagator.cpp
 *
 *  Created on: Mar 21, 2013
 *      Author: snowball
 */

#include "State/ConstantPropagator.h"

namespace borealis {

Term::Ptr ConstantPropagator::transformUnaryTerm(UnaryTermPtr term) {
    using namespace llvm;

    auto value = transform(term->getRhv());
    auto op = term->getOpcode();

    if (auto t = dyn_cast<OpaqueBoolConstantTerm>(value)) {
        return propagateTerm(op, t->getValue());
    } else if (auto t = dyn_cast<OpaqueIntConstantTerm>(value)) {
        return propagateTerm(op, t->getValue());
    } else if (auto t = dyn_cast<OpaqueFloatingConstantTerm>(value)) {
        return propagateTerm(op, t->getValue());
    } else if (auto t = dyn_cast<ConstTerm>(value)) {
        if (auto constInt = dyn_cast<ConstantInt>(t->getConstant())) {
            return propagateTerm(op, static_cast<long long>(constInt->getSExtValue()));
        } else if (auto constFloat = dyn_cast<ConstantFP>(t->getConstant())) {
            return propagateTerm(op, constFloat->getValueAPF().convertToDouble());
        }
    }
    return term;
}

Term::Ptr ConstantPropagator::transformBinaryTerm(BinaryTermPtr term) {
    auto t = transformUnifiedBinaryTerm(term->getOpcode(), term->getLhv(), term->getRhv());
    return t == nullptr ? term : t;
}

Term::Ptr ConstantPropagator::transformCmpTerm(CmpTermPtr term) {
    auto t = transformUnifiedBinaryTerm(term->getOpcode(), term->getLhv(), term->getRhv());
    return t == nullptr ? term : t;
}

} /* namespace borealis */

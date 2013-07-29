/*
 * ConstantPropagator.cpp
 *
 *  Created on: Mar 21, 2013
 *      Author: snowball
 */

#include "State/Transformer/ConstantPropagator.h"

namespace borealis {

Term::Ptr ConstantPropagator::transformUnaryTerm(UnaryTermPtr term) {
    using namespace llvm;

    auto value = term->getRhv();
    auto op = term->getOpcode();

    if (auto t = dyn_cast<OpaqueBoolConstantTerm>(value)) {
        return propagateTerm(op, t->getValue());
    } else if (auto t = dyn_cast<OpaqueIntConstantTerm>(value)) {
        return propagateTerm(op, t->getValue());
    } else if (auto t = dyn_cast<OpaqueFloatingConstantTerm>(value)) {
        return propagateTerm(op, t->getValue());
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

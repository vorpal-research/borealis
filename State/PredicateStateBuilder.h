/*
 * PredicateStateBuilder.h
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEBUILDER_H_
#define PREDICATESTATEBUILDER_H_

#include "State/PredicateStateFactory.h"

namespace borealis {

class PredicateStateBuilder {

private:

    PredicateStateFactory::Ptr PSF;
    PredicateState::Ptr State;

public:

    PredicateStateBuilder(PredicateStateFactory::Ptr PSF, PredicateState::Ptr state);
    PredicateStateBuilder(PredicateStateFactory::Ptr PSF, Predicate::Ptr pred);
    PredicateStateBuilder(const PredicateStateBuilder&) = default;
    PredicateStateBuilder(PredicateStateBuilder&&) = default;

    PredicateState::Ptr operator()() const;

    PredicateStateBuilder& operator+=(PredicateState::Ptr s);
    PredicateStateBuilder& operator+=(Predicate::Ptr p);

    friend PredicateStateBuilder operator+ (PredicateStateBuilder PSB, PredicateState::Ptr s);
    friend PredicateStateBuilder operator+ (PredicateStateBuilder PSB, Predicate::Ptr p);
    friend PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value* loc);
    friend PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value& loc);

};

PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, PredicateState::Ptr state);
PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, Predicate::Ptr p);

PredicateStateBuilder operator+ (PredicateStateBuilder PSB, PredicateState::Ptr s);
PredicateStateBuilder operator+ (PredicateStateBuilder PSB, Predicate::Ptr p);
PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value* loc);
PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value& loc);

} /* namespace borealis */

#endif /* PREDICATESTATEBUILDER_H_ */
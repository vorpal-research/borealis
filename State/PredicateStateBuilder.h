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

    PredicateStateBuilder(PredicateStateFactory::Ptr PSF);
    PredicateStateBuilder(const PredicateStateBuilder&) = default;
    PredicateStateBuilder(PredicateStateBuilder&&) = default;

    PredicateState::Ptr operator()() const;

    friend PredicateStateBuilder operator&&(PredicateStateBuilder PSB, PredicateState::Ptr s);
    friend PredicateStateBuilder operator+ (PredicateStateBuilder PSB, PredicateState::Ptr s);
    friend PredicateStateBuilder operator&&(PredicateStateBuilder PSB, Predicate::Ptr p);
    friend PredicateStateBuilder operator+ (PredicateStateBuilder PSB, Predicate::Ptr p);
    friend PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value* loc);
    friend PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value& loc);

};

PredicateStateBuilder operator&&(PredicateStateBuilder PSB, PredicateState::Ptr s);
PredicateStateBuilder operator+ (PredicateStateBuilder PSB, PredicateState::Ptr s);
PredicateStateBuilder operator&&(PredicateStateBuilder PSB, Predicate::Ptr p);
PredicateStateBuilder operator+ (PredicateStateBuilder PSB, Predicate::Ptr p);
PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value* loc);
PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value& loc);

} /* namespace borealis */

#endif /* PREDICATESTATEBUILDER_H_ */

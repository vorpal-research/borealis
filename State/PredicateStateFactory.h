/*
 * PredicateStateFactory.h
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEFACTORY_H_
#define PREDICATESTATEFACTORY_H_

#include <memory>

#include "State/BasicPredicateState.h"
#include "State/PredicateState.h"
#include "State/PredicateStateChain.h"
#include "State/PredicateStateChoice.h"

namespace borealis {

class PredicateStateFactory {

public:

    typedef std::shared_ptr<PredicateStateFactory> Ptr;

    PredicateState::Ptr Chain(
            PredicateState::Ptr base,
            PredicateState::Ptr curr);

    PredicateState::Ptr Chain(
            PredicateState::Ptr base,
            Predicate::Ptr pred);

    PredicateState::Ptr Choice(
            const std::vector<PredicateState::Ptr>& choices);

    PredicateState::Ptr Basic();

    static PredicateStateFactory::Ptr get() {
        return Ptr(new PredicateStateFactory());
    }

private:

    PredicateStateFactory() {};

};

} /* namespace borealis */

#endif /* PREDICATESTATEFACTORY_H_ */

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

namespace borealis {

class PredicateStateFactory {

public:

    typedef std::unique_ptr<PredicateStateFactory> Ptr;

    PredicateState::Ptr Chain(
            PredicateState::Ptr base,
            PredicateState::Ptr curr);

    PredicateState::Ptr Basic();

    static PredicateStateFactory::Ptr get() {
        return Ptr(new PredicateStateFactory());
    }

private:

    PredicateStateFactory() {};

};

} /* namespace borealis */

#endif /* PREDICATESTATEFACTORY_H_ */

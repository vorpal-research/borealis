/*
 * PredicateStateFactory.h
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEFACTORY_H_
#define PREDICATESTATEFACTORY_H_

#include <memory>

#include "State/PredicateState.def"
#include "State/PredicateState.h"

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
        static PredicateStateFactory::Ptr instance(new PredicateStateFactory());
        return instance;
    }

private:

    PredicateStateFactory() {};

};

} /* namespace borealis */

#endif /* PREDICATESTATEFACTORY_H_ */

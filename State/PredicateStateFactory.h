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

    using Ptr = std::shared_ptr<PredicateStateFactory>;

    PredicateState::Ptr Chain(
            PredicateState::Ptr base,
            PredicateState::Ptr curr);

    PredicateState::Ptr Chain(
            PredicateState::Ptr base,
            Predicate::Ptr pred);

    PredicateState::Ptr Choice(
            const std::vector<PredicateState::Ptr>& choices);
    PredicateState::Ptr Choice(
            std::vector<PredicateState::Ptr>&& choices);

    PredicateState::Ptr Basic();

    PredicateState::Ptr Basic(const std::vector<Predicate::Ptr>& data);
    PredicateState::Ptr Basic(std::vector<Predicate::Ptr>&& data);

    static PredicateStateFactory::Ptr get();

private:

    PredicateStateFactory() = default;

};

} /* namespace borealis */

#endif /* PREDICATESTATEFACTORY_H_ */

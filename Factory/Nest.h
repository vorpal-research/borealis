/*
 * Nest.h
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#ifndef FACTORY_NEST_H_
#define FACTORY_NEST_H_

#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"
#include "Type/TypeFactory.h"
#include "Util/slottracker.h"

namespace borealis {

class FactoryNest {

public:

    std::shared_ptr<TypeFactory> Type;
    std::shared_ptr<TermFactory> Term;
    std::shared_ptr<PredicateFactory> Predicate;
    std::shared_ptr<PredicateStateFactory> State;

    FactoryNest();
    FactoryNest(const FactoryNest&);
    FactoryNest(FactoryNest&&);
    ~FactoryNest();

    FactoryNest& operator=(const FactoryNest&);
    FactoryNest& operator=(FactoryNest&&);

    FactoryNest(SlotTracker* st);

};

} /* namespace borealis */

#endif /* FACTORY_NEST_H_ */

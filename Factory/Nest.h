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

namespace borealis {

class FactoryNest {

public:

    TypeFactory::Ptr Type;
    TermFactory::Ptr Term;
    PredicateFactory::Ptr Predicate;
    PredicateStateFactory::Ptr State;

    FactoryNest() {};
    FactoryNest(const FactoryNest&) = default;
    FactoryNest(FactoryNest&&) = default;

    FactoryNest& operator=(const FactoryNest&) = default;
    FactoryNest& operator=(FactoryNest&&) = default;

    FactoryNest(SlotTracker* st) :
        Type(TypeFactory::get()),
        Term(TermFactory::get(st, Type)),
        Predicate(PredicateFactory::get()),
        State(PredicateStateFactory::get()) {};

};

} /* namespace borealis */

#endif /* FACTORY_NEST_H_ */

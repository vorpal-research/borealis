/*
 * Nest.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "Factory/Nest.h"

namespace borealis {

FactoryNest::FactoryNest() {};
FactoryNest::FactoryNest(const FactoryNest&) = default;
FactoryNest::FactoryNest(FactoryNest&&) = default;
FactoryNest::~FactoryNest() {};

FactoryNest& FactoryNest::operator=(const FactoryNest&) = default;
FactoryNest& FactoryNest::operator=(FactoryNest&&) = default;

FactoryNest::FactoryNest(SlotTracker* st) :
    Type(TypeFactory::get()),
    Term(TermFactory::get(st, Type)),
    Predicate(PredicateFactory::get()),
    State(PredicateStateFactory::get()) {};

}

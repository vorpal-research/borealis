/*
 * Nest.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "Factory/Nest.h"
#include "State/PredicateStateBuilder.h"

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

PredicateState::Ptr FactoryNest::getGlobalState(llvm::Module* M) {
    auto& globals = M->getGlobalList();
    Predicate::Ptr gPredicate = Predicate->getGlobalsPredicate(
        util::viewContainer(globals)
            .map([this](llvm::GlobalVariable& g) { return Term->getValueTerm(&g); })
            .toVector()
    );
    return (State * gPredicate)();
}

} // namespace borealis

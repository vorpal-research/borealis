/*
 * Nest.h
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#ifndef FACTORY_NEST_H_
#define FACTORY_NEST_H_

#include <llvm/Module.h>

#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"
#include "Type/TypeFactory.h"
#include "Util/slottracker.h"

namespace borealis {

class FactoryNest {

public:

    TypeFactory::Ptr Type;
    TermFactory::Ptr Term;
    PredicateFactory::Ptr Predicate;
    PredicateStateFactory::Ptr State;

    FactoryNest();
    FactoryNest(const FactoryNest&);
    FactoryNest(FactoryNest&&);
    ~FactoryNest();

    FactoryNest& operator=(const FactoryNest&);
    FactoryNest& operator=(FactoryNest&&);

    FactoryNest(SlotTracker* st);

    PredicateState::Ptr getGlobalState(llvm::Module* M);

};

} /* namespace borealis */

#endif /* FACTORY_NEST_H_ */

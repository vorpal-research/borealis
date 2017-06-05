/*
 * Nest.h
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#ifndef FACTORY_NEST_H_
#define FACTORY_NEST_H_

#include <llvm/IR/Module.h>
#include <Util/ir_writer.h>

#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"
#include "Type/TypeFactory.h"
#include "Util/slottracker.h"

namespace borealis {

class FactoryNest {

public:

    SlotTracker* Slot;
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

    FactoryNest(const llvm::DataLayout* DL, SlotTracker* st);

    PredicateState::Ptr getGlobalState(const llvm::Function* F);

};

} /* namespace borealis */

#endif /* FACTORY_NEST_H_ */

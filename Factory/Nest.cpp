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

    auto gs = util::viewContainer(globals)
        .map([this](llvm::GlobalVariable& g) { return Term->getValueTerm(&g); })
        .toVector();

    auto gPredicate = Predicate->getGlobalsPredicate(gs);

    auto seqDataPredicates = util::viewContainer(globals)
        .filter(llvm::isaer<llvm::ConstantDataSequential>())
        .map(llvm::caster<llvm::ConstantDataSequential>())
        .map([this](llvm::ConstantDataSequential& d) -> Predicate::Ptr {
            auto base = Term->getValueTerm(&d);
            auto data = util::range(0U, d.getNumElements())
                .map([this,&d](unsigned i) { return d.getElementAsConstant(i); })
                .map([this](llvm::Constant* c) { return Term->getValueTerm(c); })
                .toVector();
            return Predicate->getSeqDataPredicate(base, data);
        })
        .toVector();

    return (State * gPredicate + seqDataPredicates)();
}

} // namespace borealis

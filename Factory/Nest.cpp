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

Predicate::Ptr fromGlobalVariable(FactoryNest& FN, llvm::GlobalVariable& gv) {
    using namespace llvm;

    auto base = FN.Term->getValueTerm(&gv);

    if (gv.hasInitializer()) {
        auto* ini = gv.getInitializer();

        if (auto* cds = dyn_cast<ConstantDataSequential>(ini)) {
            auto data = util::range(0U, cds->getNumElements())
                .map([&cds](unsigned i) { return cds->getElementAsConstant(i); })
                .map([&FN](Constant* c) { return FN.Term->getValueTerm(c); })
                .toVector();
            return FN.Predicate->getSeqDataPredicate(base, data);

        } else if (auto* caz = dyn_cast<ConstantAggregateZero>(ini)) {
            auto numElements = 1U;
            if (auto* type = dyn_cast<StructType>(caz->getType())) {
                numElements = type->getNumElements();
            } else if (auto* type = dyn_cast<ArrayType>(caz->getType())) {
                numElements = type->getNumElements();
            } else if (auto* type = dyn_cast<VectorType>(caz->getType())) {
                numElements = type->getNumElements();
            }

            auto data = util::range(0U, numElements)
                .map([&caz](unsigned i) { return caz->getElementValue(i); })
                .map([&FN](Constant* c) { return FN.Term->getValueTerm(c); })
                .toVector();
            return FN.Predicate->getSeqDataPredicate(base, data);

        }
    }

    return FN.Predicate->getGlobalsPredicate({base});
}

PredicateState::Ptr FactoryNest::getGlobalState(llvm::Module* M) {
    auto& globals = M->getGlobalList();

    // FIXME: Marker predicate
    auto init = Predicate->getGlobalsPredicate({});

    auto gvPredicates = util::viewContainer(globals)
        .map([this](llvm::GlobalVariable& gv) { return fromGlobalVariable(*this, gv); })
        .toVector();

    return (State * init + gvPredicates)();
}

} // namespace borealis

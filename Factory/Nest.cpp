/*
 * Nest.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "Factory/Nest.h"
#include "State/PredicateStateBuilder.h"

#include "Util/functional.hpp"
#include "Util/macros.h"

namespace borealis {

static const llvm::DataLayout* defaultLayout() {
    static llvm::DataLayout dl("");
    return &dl;
}

FactoryNest::FactoryNest():
    FactoryNest(defaultLayout(), (SlotTracker*)nullptr){}

FactoryNest::FactoryNest(const FactoryNest&) = default;
FactoryNest::FactoryNest(FactoryNest&&) = default;
FactoryNest::~FactoryNest() = default;

FactoryNest& FactoryNest::operator=(const FactoryNest&) = default;
FactoryNest& FactoryNest::operator=(FactoryNest&&) = default;

FactoryNest::FactoryNest(const llvm::DataLayout* DL, SlotTracker* st) :
    Slot(st),
    Type(TypeFactory::get()),
    Term(TermFactory::get(Slot, DL, Type)),
    Predicate(PredicateFactory::get()),
    State(PredicateStateFactory::get()) {};

Predicate::Ptr fromGlobalVariable(FactoryNest& FN, const llvm::GlobalVariable& gv) {
    using namespace llvm;

    auto&& base = FN.Term->getValueTerm(&gv);

    if (gv.hasInitializer()) {
        auto* ini = gv.getInitializer();

        auto&& numElements = TypeUtils::getTypeSizeInElems(
            FN.Type->cast(ini->getType(), gv.getParent()->getDataLayout())
        );

        if (auto* cds = dyn_cast<ConstantDataSequential>(ini)) {
            auto&& data = util::range(0ULL, numElements)
                    .map(APPLY(cds->getElementAsConstant))
                    .map(APPLY(FN.Term->getValueTerm))
                    .toVector();
            return FN.Predicate->getSeqDataPredicate(base, data);

        } else if (dyn_cast<ConstantAggregateZero>(ini)) {
            return FN.Predicate->getSeqDataZeroPredicate(base, numElements);

        } else if (auto* c = dyn_cast<Constant>(ini)) {
            auto&& data = util::viewContainer(getAsSeqData(c))
                    .map(APPLY(FN.Term->getValueTerm))
                    .toVector();
            ASSERTC(numElements == data.size());
            return FN.Predicate->getSeqDataPredicate(base, data);
        }
    }

    return FN.Predicate->getGlobalsPredicate({base});
}

PredicateState::Ptr FactoryNest::getGlobalState(const llvm::Module* M) {
    auto&& globals = M->getGlobalList();

    // FIXME: Marker predicate
    auto&& init = Predicate->getGlobalsPredicate({});

    auto&& gvPredicates = util::viewContainer(globals)
            .map([&](auto&& gv) { return fromGlobalVariable(*this, gv); })
            .toVector();

    return (State * init + gvPredicates)();
}

} // namespace borealis

#include "Util/unmacros.h"

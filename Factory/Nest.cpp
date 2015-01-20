/*
 * Nest.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "Factory/Nest.h"
#include "State/PredicateStateBuilder.h"

#include "Util/macros.h"

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

Predicate::Ptr fromGlobalVariable(FactoryNest& FN, const llvm::GlobalVariable& gv) {
    using namespace llvm;

    auto base = FN.Term->getValueTerm(&gv);

    if (gv.hasInitializer()) {
        auto* ini = gv.getInitializer();

        auto numElements = TypeUtils::getTypeSizeInElems(
            FN.Type->cast(ini->getType())
        );

        if (auto* cds = dyn_cast<ConstantDataSequential>(ini)) {
            auto data = util::range(0ULL, numElements)
                .map([&cds](unsigned long long i) { return cds->getElementAsConstant(i); })
                .map([&FN](const Constant* c) { return FN.Term->getValueTerm(c); })
                .toVector();
            return FN.Predicate->getSeqDataPredicate(base, data);

        } else if (auto* _ = dyn_cast<ConstantAggregateZero>(ini)) {
            util::use(_);
            auto zero = FN.Term->getIntTerm(0);
            auto data = util::range(0ULL, numElements)
                .map([&zero](unsigned long long _) { return util::use(_), zero; })
                .toVector();
            return FN.Predicate->getSeqDataPredicate(base, data);

        } else if (auto* c = dyn_cast<Constant>(ini)) {
            auto data = util::viewContainer(getAsSeqData(c))
                .map([&FN](const Constant* c) { return FN.Term->getValueTerm(c); })
                .toVector();
            ASSERTC(numElements == data.size());
            return FN.Predicate->getSeqDataPredicate(base, data);
        }
    }

    return FN.Predicate->getGlobalsPredicate({base});
}

PredicateState::Ptr FactoryNest::getGlobalState(const llvm::Module* M) {
    auto& globals = M->getGlobalList();

    // FIXME: Marker predicate
    auto init = Predicate->getGlobalsPredicate({});

    auto gvPredicates = util::viewContainer(globals)
        .map([this](const llvm::GlobalVariable& gv) { return fromGlobalVariable(*this, gv); })
        .toVector();

    return (State * init + gvPredicates)();
}

} // namespace borealis

#include "Util/unmacros.h"

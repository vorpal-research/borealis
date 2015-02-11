/*
 * PredicateStateChain.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "State/PredicateStateChain.h"

#include "Util/macros.h"

namespace borealis {

PredicateStateChain::PredicateStateChain(PredicateState::Ptr base, PredicateState::Ptr curr) :
        PredicateState(class_tag<Self>()),
        base(base),
        curr(curr) {
    ASSERTC(base != nullptr);
    ASSERTC(curr != nullptr);
}

PredicateState::Ptr PredicateStateChain::getBase() const {
    return base;
}

PredicateState::Ptr PredicateStateChain::getCurr() const {
    return curr;
}

PredicateState::Ptr PredicateStateChain::addPredicate(Predicate::Ptr pred) const {
    ASSERTC(pred != nullptr);

    return Simplified<Self>(
        this->base,
        this->curr + pred
    );
}

PredicateState::Ptr PredicateStateChain::addVisited(const Locus& locus) const {
    return Simplified<Self>(
        this->base,
        this->curr << locus
    );
}

bool PredicateStateChain::hasVisited(std::initializer_list<Locus> loci) const {
    auto&& visited = std::unordered_set<Locus>(loci.begin(), loci.end());
    return hasVisitedFrom(visited);
}

bool PredicateStateChain::hasVisitedFrom(Loci& visited) const {
    return curr->hasVisitedFrom(visited) || base->hasVisitedFrom(visited);
}

PredicateState::Loci PredicateStateChain::getVisited() const {
    Loci res;
    auto&& baseLoci = base->getVisited();
    res.insert(baseLoci.begin(), baseLoci.end());
    auto&& currLoci = curr->getVisited();
    res.insert(currLoci.begin(), currLoci.end());
    return res;
}

PredicateStateChain::SelfPtr PredicateStateChain::fmap_(FMapper f) const {
    return Uniquified(
        f(base),
        f(curr)
    );
}

PredicateState::Ptr PredicateStateChain::fmap(FMapper f) const {
    return Simplified(fmap_(f).release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> PredicateStateChain::splitByTypes(
        std::initializer_list<PredicateType> types) const {
    auto&& baseSplit = base->splitByTypes(types);
    auto&& currSplit = curr->splitByTypes(types);

    return std::make_pair(
        Simplified<Self>(baseSplit.first, currSplit.first),
        Simplified<Self>(baseSplit.second, currSplit.second)
    );
}

PredicateState::Ptr PredicateStateChain::sliceOn(PredicateState::Ptr on) const {
    if (*base == *on) {
        return curr;
    }

    auto&& slice = base->sliceOn(on);
    if (slice != nullptr) {
        return Simplified<Self>(slice, curr);
    }

    return nullptr;
}

PredicateState::Ptr PredicateStateChain::simplify() const {
    auto&& res = fmap_([](auto&& s) { return s->simplify(); });

    if (res->curr->isEmpty()) {
        return res->base;
    }
    if (res->base->isEmpty()) {
        return res->curr;
    }

    return PredicateState::Ptr(res.release());
}

bool PredicateStateChain::isEmpty() const {
    return curr->isEmpty() && base->isEmpty();
}

bool PredicateStateChain::equals(const PredicateState* other) const {
    if (auto* o = llvm::dyn_cast_or_null<Self>(other)) {
        return PredicateState::equals(other) &&
                *this->base == *o->base &&
                *this->curr == *o->curr;
    } else return false;
}

borealis::logging::logstream& PredicateStateChain::dump(borealis::logging::logstream& s) const {
    return s << base << "->" << curr;
}

std::string PredicateStateChain::toString() const {
    return base->toString() + "->" + curr->toString();
}

} /* namespace borealis */

#include "Util/unmacros.h"

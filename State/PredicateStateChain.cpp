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
};

PredicateState::Ptr PredicateStateChain::addPredicate(Predicate::Ptr pred) const {
    ASSERTC(pred != nullptr);

    return Simplified(new Self{
        this->base,
        this->curr + pred
    });
}

PredicateState::Ptr PredicateStateChain::addVisited(const llvm::Value* loc) const {
    return Simplified(new Self{
        this->base,
        this->curr << loc
    });
}

bool PredicateStateChain::hasVisited(std::initializer_list<const llvm::Value*> locs) const {
    auto visited = std::unordered_set<const llvm::Value*>(locs.begin(), locs.end());
    return hasVisitedFrom(visited);
}

bool PredicateStateChain::hasVisitedFrom(std::unordered_set<const llvm::Value*>& visited) const {
    return curr->hasVisitedFrom(visited) || base->hasVisitedFrom(visited);
}

PredicateStateChain::SelfPtr PredicateStateChain::fmap_(FMapper f) const {
    return SelfPtr(new Self{
        f(this->base),
        f(this->curr)
    });
}

PredicateState::Ptr PredicateStateChain::fmap(FMapper f) const {
    return Simplified(fmap_(f).release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> PredicateStateChain::splitByTypes(std::initializer_list<PredicateType> types) const {
    auto baseSplit = this->base->splitByTypes(types);
    auto currSplit = this->curr->splitByTypes(types);

    return std::make_pair(
        Simplified(new Self{ baseSplit.first, currSplit.first }),
        Simplified(new Self{ baseSplit.second, currSplit.second })
    );
}

PredicateState::Ptr PredicateStateChain::sliceOn(PredicateState::Ptr base) const {
    if (*this->base == *base) {
        return this->curr;
    }

    auto slice = this->base->sliceOn(base);
    if (slice != nullptr) {
        return Simplified(new Self{ slice, this->curr });
    }

    return nullptr;
}

PredicateState::Ptr PredicateStateChain::simplify() const {
    auto res = fmap_([&](const PredicateState::Ptr& s) { return s->simplify(); });

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

borealis::logging::logstream& PredicateStateChain::dump(borealis::logging::logstream& s) const {
    return s << base << "->" << curr;
}

std::string PredicateStateChain::toString() const {
    return base->toString() + "->" + curr->toString();
}

} /* namespace borealis */

#include "Util/unmacros.h"

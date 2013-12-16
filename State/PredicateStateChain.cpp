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

PredicateState::Ptr PredicateStateChain::addVisited(const Locus& l) const {
    return Simplified(new Self{
        this->base,
        this->curr << l
    });
}

bool PredicateStateChain::hasVisited(std::initializer_list<Locus> ls) const {
    auto visited = std::unordered_set<Locus>(ls.begin(), ls.end());
    return hasVisitedFrom(visited);
}

bool PredicateStateChain::hasVisitedFrom(Locs& visited) const {
    return curr->hasVisitedFrom(visited) || base->hasVisitedFrom(visited);
}

PredicateState::Locs PredicateStateChain::getVisited() const {
    Locs res;
    auto baseLocs = base->getVisited();
    res.insert(baseLocs.begin(), baseLocs.end());
    auto currLocs = curr->getVisited();
    res.insert(currLocs.begin(), currLocs.end());
    return res;
}

PredicateStateChain::SelfPtr PredicateStateChain::fmap_(FMapper f) const {
    return util::uniq(new Self{
        f(base),
        f(curr)
    });
}

PredicateState::Ptr PredicateStateChain::fmap(FMapper f) const {
    return Simplified(fmap_(f).release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> PredicateStateChain::splitByTypes(std::initializer_list<PredicateType> types) const {
    auto baseSplit = base->splitByTypes(types);
    auto currSplit = curr->splitByTypes(types);

    return std::make_pair(
        Simplified(new Self{ baseSplit.first, currSplit.first }),
        Simplified(new Self{ baseSplit.second, currSplit.second })
    );
}

PredicateState::Ptr PredicateStateChain::sliceOn(PredicateState::Ptr on) const {
    if (*base == *on) {
        return curr;
    }

    auto slice = base->sliceOn(on);
    if (slice != nullptr) {
        return Simplified(new Self{ slice, curr });
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

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
        PredicateState(type_id<Self>()),
        base(base),
        curr(curr) {};

PredicateState::Ptr PredicateStateChain::addPredicate(Predicate::Ptr pred) const {
    ASSERTC(pred != nullptr);

    auto* res = new Self(*this);
    res->curr = res->curr + pred;
    return PredicateState::Ptr(res);
}

logic::Bool PredicateStateChain::toZ3(Z3ExprFactory& z3ef, ExecutionContext* pctx) const {
    TRACE_FUNC;
    return base->toZ3(z3ef, pctx) && curr->toZ3(z3ef, pctx);
}

PredicateState::Ptr PredicateStateChain::addVisited(const llvm::Value* loc) const {
    auto* res = new Self(*this);
    res->curr = res->curr << loc;
    return PredicateState::Ptr(res);
}

bool PredicateStateChain::hasVisited(std::initializer_list<const llvm::Value*> locs) const {
    // FIXME: akhin Just fix this piece of crap
    for (const auto* loc : locs) {
        if (curr->hasVisited({loc}) || base->hasVisited({loc})) continue;
        else return false;
    }
    return true;
}

PredicateState::Ptr PredicateStateChain::map(Mapper m) const {
    auto* res = new Self(*this);
    res->base = res->base->map(m);
    res->curr = res->curr->map(m);
    return PredicateState::Ptr(res);
}

PredicateState::Ptr PredicateStateChain::filterByTypes(std::initializer_list<PredicateType> types) const {
    auto* res = new Self(*this);
    res->base = res->base->filterByTypes(types);
    res->curr = res->curr->filterByTypes(types);
    return PredicateState::Ptr(res);
}

PredicateState::Ptr PredicateStateChain::filter(Filterer f) const {
    auto* res = new Self(*this);
    res->base = res->base->filter(f);
    res->curr = res->curr->filter(f);
    return PredicateState::Ptr(res);
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> PredicateStateChain::splitByTypes(std::initializer_list<PredicateType> types) const {
    auto baseSplit = this->base->splitByTypes(types);
    auto currSplit = this->curr->splitByTypes(types);

    return std::make_pair(
        PredicateState::Ptr(new Self(baseSplit.first, currSplit.first)),
        PredicateState::Ptr(new Self(baseSplit.second, currSplit.second))
    );
}

PredicateState::Ptr PredicateStateChain::sliceOn(PredicateState::Ptr base) const {
    if (*this->base == *base) {
        return this->curr;
    }

    auto slice = this->base->sliceOn(base);
    if (slice) {
        return PredicateState::Ptr(new Self(slice, this->curr));
    }

    return nullptr;
}

bool PredicateStateChain::isEmpty() const {
    return curr->isEmpty() && base->isEmpty();
}

std::string PredicateStateChain::toString() const {
    return curr->toString() + "<-" + base->toString();
}

} /* namespace borealis */

#include "Util/unmacros.h"

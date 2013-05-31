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

    auto res = SelfPtr(new Self(*this));
    res->curr = res->curr + pred;
    return Simplified(res.release());
}

logic::Bool PredicateStateChain::toZ3(Z3ExprFactory& z3ef, ExecutionContext* pctx) const {
    TRACE_FUNC;
    auto res = z3ef.getTrue();
    res = res && base->toZ3(z3ef, pctx);
    res = res && curr->toZ3(z3ef, pctx);
    return res;
}

PredicateState::Ptr PredicateStateChain::addVisited(const llvm::Value* loc) const {
    auto res = SelfPtr(new Self(*this));
    res->curr = res->curr << loc;
    return Simplified(res.release());
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
    auto res = SelfPtr(new Self(*this));
    res->base = res->base->map(m);
    res->curr = res->curr->map(m);
    return Simplified(res.release());
}

PredicateState::Ptr PredicateStateChain::filterByTypes(std::initializer_list<PredicateType> types) const {
    auto res = SelfPtr(new Self(*this));
    res->base = res->base->filterByTypes(types);
    res->curr = res->curr->filterByTypes(types);
    return Simplified(res.release());
}

PredicateState::Ptr PredicateStateChain::filter(Filterer f) const {
    auto res = SelfPtr(new Self(*this));
    res->base = res->base->filter(f);
    res->curr = res->curr->filter(f);
    return Simplified(res.release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> PredicateStateChain::splitByTypes(std::initializer_list<PredicateType> types) const {
    auto baseSplit = this->base->splitByTypes(types);
    auto currSplit = this->curr->splitByTypes(types);

    return std::make_pair(
        Simplified(new Self(baseSplit.first, currSplit.first)),
        Simplified(new Self(baseSplit.second, currSplit.second))
    );
}

PredicateState::Ptr PredicateStateChain::sliceOn(PredicateState::Ptr base) const {
    if (*this->base == *base) {
        return this->curr;
    }

    auto slice = this->base->sliceOn(base);
    if (slice) {
        return Simplified(new Self(slice, this->curr));
    }

    return nullptr;
}

PredicateState::Ptr PredicateStateChain::simplify() const {
    auto res = SelfPtr(new Self(*this));
    res->base = res->base->simplify();
    res->curr = res->curr->simplify();

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

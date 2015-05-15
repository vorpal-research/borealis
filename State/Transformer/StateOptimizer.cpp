/*
 * StateOptimizer.cpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#include "State/Transformer/StateOptimizer.h"

#include "Util/cast.hpp"
#include "Util/util.h"

#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

StateOptimizer::StateOptimizer(FactoryNest FN) : Base(FN) {}

PredicateState::Ptr StateOptimizer::transformPredicateStateChain(PredicateStateChainPtr ps) {

    auto&& base = ps->getBase();
    auto&& curr = ps->getCurr();

    PredicateState::Ptr res;
    if (auto&& merged = merge(base, curr)) {
        res = merged;
    } else if (auto&& m = util::match_tuple<PredicateStateChain, BasicPredicateState>::doit(base, curr)) {
        if (auto&& merged = merge(m->get<0>()->getCurr(), m->get<1>()->self())) {
            res = FN.State->Chain(
                m->get<0>()->getBase(),
                merged
            );
            res = transformPredicateStateChain(std::static_pointer_cast<const PredicateStateChain>(res));
        }
    } else if (auto&& m = util::match_tuple<BasicPredicateState, PredicateStateChain>::doit(base, curr)) {
        if (auto&& merged = merge(m->get<0>()->self(), m->get<1>()->getBase())) {
            res = FN.State->Chain(
                merged,
                m->get<1>()->getCurr()
            );
            res = transformPredicateStateChain(std::static_pointer_cast<const PredicateStateChain>(res));
        }
    }

    return nullptr == res ? ps : res;
}

PredicateState::Ptr StateOptimizer::transformBasic(BasicPredicateStatePtr ps) {
    return ps;
}

PredicateState::Ptr StateOptimizer::transformBasicPredicateState(BasicPredicateStatePtr ps) {
    return ps;
}

PredicateState::Ptr StateOptimizer::transformBase(PredicateState::Ptr ps) {
    if (auto&& v = util::at(cache, ps)) return v.getUnsafe();
    return cache[ps] = Base::transformBase(ps);
}

Predicate::Ptr StateOptimizer::transformBase(Predicate::Ptr pred) {
    return pred;
}

PredicateState::Ptr StateOptimizer::transform(PredicateState::Ptr ps) {
    return transformBase(ps)->simplify();
}

PredicateState::Ptr StateOptimizer::merge(PredicateState::Ptr a, PredicateState::Ptr b) {
    auto&& mergeKey = std::make_pair(a, b);

    if (auto&& m = util::at(mergeCache, mergeKey)) {
        return m.getUnsafe();
    } else if (auto&& m = util::match_tuple<BasicPredicateState, BasicPredicateState>::doit(a, b)) {
        return mergeCache[mergeKey] = FN.State->Basic(m->get<0>()->getData() + m->get<1>()->getData());
    } else {
        return mergeCache[mergeKey] = nullptr;
    }
}

} /* namespace borealis */

#include "Util/unmacros.h"

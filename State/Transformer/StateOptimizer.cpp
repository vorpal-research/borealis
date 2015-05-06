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
    if (auto&& v = util::at(cache, ps)) return v.getUnsafe();

    auto&& base = util::at(cache, ps->getBase()).getOrElse(ps->getBase());
    auto&& curr = util::at(cache, ps->getCurr()).getOrElse(ps->getCurr());

    PredicateState::Ptr res;
    if (auto&& merged = merge(base, curr)) {
        res = merged;
    } else if (auto&& m = util::match_tuple<PredicateStateChain, BasicPredicateState>::doit(base, curr)) {
        if (auto&& merged = merge(m->get<0>()->getCurr(), m->get<1>()->self())) {
            res = FN.State->Chain(
                m->get<0>()->getBase(),
                merged
            );
        }
    } else if (auto&& m = util::match_tuple<BasicPredicateState, PredicateStateChain>::doit(base, curr)) {
        if (auto&& merged = merge(m->get<0>()->self(), m->get<1>()->getBase())) {
            res = FN.State->Chain(
                merged,
                m->get<1>()->getCurr()
            );
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

Predicate::Ptr StateOptimizer::transformBase(Predicate::Ptr pred) {
    return pred;
}

PredicateState::Ptr StateOptimizer::transform(PredicateState::Ptr ps) {
    if (auto&& v = util::at(cache, ps)) return v.getUnsafe();
    return cache[ps] = transformBase(ps)->simplify();
}

PredicateState::Ptr StateOptimizer::merge(PredicateState::Ptr a, PredicateState::Ptr b) {
    if (auto&& m = util::match_tuple<BasicPredicateState, BasicPredicateState>::doit(a, b)) {
        return FN.State->Basic(m->get<0>()->getData() + m->get<1>()->getData());
    }
    return nullptr;
}

} /* namespace borealis */

#include "Util/unmacros.h"

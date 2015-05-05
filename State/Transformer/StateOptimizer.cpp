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
    PredicateState::Ptr res;
    if (auto&& merged = merge(ps->getBase(), ps->getCurr())) {
        res = merged;
    } else if (auto&& m = util::match_tuple<PredicateStateChain, BasicPredicateState>::doit(ps->getBase(), ps->getCurr())) {
        if (auto&& merged = merge(m->get<0>()->getCurr(), m->get<1>()->self())) {
            res = FN.State->Chain(
                m->get<0>()->getBase(),
                merged
            );
        }
    } else if (auto&& m = util::match_tuple<BasicPredicateState, PredicateStateChain>::doit(ps->getBase(), ps->getCurr())) {
        if (auto&& merged = merge(m->get<0>()->self(), m->get<1>()->getBase())) {
            res = FN.State->Chain(
                merged,
                m->get<1>()->getCurr()
            );
        }
    }
    return nullptr == res ? ps : transform(res);
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
    return transformBase(ps);
}

PredicateState::Ptr StateOptimizer::merge(PredicateState::Ptr a, PredicateState::Ptr b) {
    if (auto&& m = util::match_tuple<BasicPredicateState, BasicPredicateState>::doit(a, b)) {
        return FN.State->Basic(m->get<0>()->getData() + m->get<1>()->getData());
    }
    return nullptr;
}

} /* namespace borealis */

#include "Util/unmacros.h"

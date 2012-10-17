/*
 * PredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "PredicateState.h"

#include "../util.h"

namespace borealis {

using util::for_each;

PredicateState::PredicateState() {
}

PredicateState::PredicateState(const PredicateState& state) :
    data(state.data) {
}

PredicateState::PredicateState(PredicateState&& state) :
    data(std::move(state.data)) {
}

const PredicateState& PredicateState::operator=(const PredicateState& state) {
    data = state.data;
    return *this;
}

const PredicateState& PredicateState::operator=(PredicateState&& state) {
    data = std::move(state.data);
    return *this;
}

PredicateState PredicateState::addPredicate(const Predicate* pred) const {
    PredicateState res = PredicateState(*this);
    res.data[pred->getKey()] = pred;
    return res;
}

PredicateState PredicateState::merge(const PredicateState& state) const {
    PredicateState res = PredicateState(*this);
    for_each(state, [this, &res](const DataEntry& entry){
        res.data[entry.first] = entry.second;
    });
    return res;
}

std::pair<z3::expr, z3::expr> PredicateState::toZ3(z3::context& ctx) const {
    using namespace::std;
    using namespace::z3;

    auto from = vector<expr>();
    auto to = vector<expr>();

    for(auto& entry : data) {
        auto v = entry.second;
        if (v->getType() == PredicateType::PATH) {
            from.push_back(v->toZ3(ctx));
        } else if (v->getType() == PredicateType::STATE) {
            to.push_back(v->toZ3(ctx));
        }
    }

    auto f = ctx.bool_val(true);
    for(auto& e : from) {
        f = f && e;
    }

    auto t = ctx.bool_val(true);
    for(auto& e : to) {
        t = t && e;
    }

    return make_pair(f, t);
}

PredicateState::~PredicateState() {
    // TODO
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateState& state) {
    s << '(';
    if (!state.empty()) {
        auto iter = state.begin();
        const PredicateState::DataEntry& el = *iter++;
        s << *(el.second);
        std::for_each(iter, state.end(), [&s](const PredicateState::DataEntry& e){
            s << ',' << *(e.second);
        });
    }
    s << ')';
    return s;
}

} /* namespace borealis */

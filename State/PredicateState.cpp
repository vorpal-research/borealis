/*
 * PredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "PredicateState.h"

#include "../util.h"

namespace borealis {

using util::contains;
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

    auto ds = Predicate::DependeeSet();
    ds.insert(pred->getDependee());
    res.removeDependants(ds);

    res.data[pred->getKey()] = pred;
    return res;
}

PredicateState PredicateState::merge(const PredicateState& state) const {
    PredicateState res = PredicateState(*this);

    auto ds = Predicate::DependeeSet();
    for(auto& p : state) {
        ds.insert(p.second->getDependee());
    }
    res.removeDependants(ds);

    for_each(state, [this, &res](const DataEntry& entry){
        res.data[entry.first] = entry.second;
    });
    return res;
}

std::pair<z3::expr, z3::expr> PredicateState::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::std;
    using namespace::z3;

    auto path = vector<expr>();
    auto state = vector<expr>();

    for(auto& entry : data) {
        auto v = entry.second;
        if (v->getType() == PredicateType::PATH) {
            path.push_back(v->toZ3(z3ef));
        } else if (v->getType() == PredicateType::STATE) {
            state.push_back(v->toZ3(z3ef));
        }
    }

    auto p = z3ef.getBoolConst(true);
    for(auto& e : path) {
        p = p && e;
    }

    auto s = z3ef.getBoolConst(true);;
    for(auto& e : state) {
        s = s && e;
    }

    return make_pair(p, s);
}

void PredicateState::removeDependants(Predicate::DependeeSet dependees) {
    auto iter = data.begin();
    while (true)
    {
next:
        if (iter == data.end()) break;
        auto e = *iter;
        auto ds = e.second->getDependees();
        for(auto& d : ds) {
            if (contains(dependees, d)) {
                iter = data.erase(iter);
                goto next;
            }
        }
        ++iter;
    }
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

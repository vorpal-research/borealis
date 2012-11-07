/*
 * PredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "PredicateState.h"

namespace borealis {

using util::contains;
using util::view;

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
    for (const auto& p : state) {
        ds.insert(p.second->getDependee());
    }
    res.removeDependants(ds);

    for (const auto& entry : state) {
        res.data[entry.first] = entry.second;
    }
    return res;
}

std::pair<z3::expr, z3::expr> PredicateState::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    auto path = std::vector<expr>();
    auto state = std::vector<expr>();

    for(auto& entry : data) {
        auto v = entry.second;
        if (v->getType() == PredicateType::PATH) {
            path.push_back(v->toZ3(z3ef));
        } else if (v->getType() == PredicateType::STATE) {
            state.push_back(v->toZ3(z3ef));
        }
    }

    auto p = z3ef.getBoolConst(true);
    for (const auto& e : path) {
        p = p && e;
    }

    auto s = z3ef.getBoolConst(true);;
    for (const auto& e : state) {
        s = s && e;
    }

    return std::make_pair(p, s);
}

void PredicateState::removeDependants(Predicate::DependeeSet dependees) {
    auto iter = data.begin();
    while (true)
    {
        next:
        if (iter == data.end()) break;
        auto e = *iter;
        auto ds = e.second->getDependees();
        for (const auto& d : ds) {
            if (contains(dependees, d)) {
                iter = data.erase(iter);
                goto next;
            }
        }
        ++iter;
    }
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateState& state) {
    using borealis::util::streams::endl;
    s << '(';
    if (!state.empty()) {
        auto iter = state.begin();
        const PredicateState::DataEntry& el = *iter++;
        s << endl << "  " << *(el.second);
        for (const auto& e : view(iter, state.end())) {
            s << ',' << endl << "  " << *(e.second);
        }
    }
    s << endl << ')';
    return s;
}

logging::stream_t& operator<<(logging::stream_t& s, const PredicateState& state) {
    s << '(';
    if (!state.empty()) {
        auto iter = state.begin();
        const PredicateState::DataEntry& el = *iter++;
        s << endl << "  " << *(el.second);
        for (const auto& e : view(iter, state.end())) {
            s << ',' << endl << "  " << *(e.second);
        }
    }
    s << endl << ')';
    return s;
}

} /* namespace borealis */

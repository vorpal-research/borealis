/*
 * PredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "PredicateState.h"

#include "Logging/tracer.hpp"
#include "Solver/Z3Context.h"

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

PredicateState PredicateState::addPredicate(Predicate::Ptr pred) const {
    PredicateState res = PredicateState(*this);

    if (contains(data, pred)) return res;

    auto ds = Predicate::DependeeSet();
    ds.insert(pred->getDependee());
    res.removeDependants(ds);

    res.data.push_back(pred);
    return res;
}

std::pair<z3::expr, z3::expr> PredicateState::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    TRACE_FUNC;

    Z3Context ctx(z3ef);

    auto path = std::vector<expr>();
    auto state = std::vector<expr>();

    for (auto& v : data) {
        if (v->getType() == PredicateType::PATH) {
            path.push_back(v->toZ3(z3ef, &ctx));
        } else if (v->getType() == PredicateType::STATE) {
            state.push_back(v->toZ3(z3ef, &ctx));
        }
    }

    auto p = z3ef.getBoolConst(true);
    for (const auto& e : path) {
        p = p && e;
    }

    auto s = z3ef.getBoolConst(true);
    for (const auto& e : state) {
        s = s && e;
    }
    s = s && ctx.toZ3();

    return std::make_pair(p, s);
}

void PredicateState::removeDependants(Predicate::DependeeSet dependees) {
    auto iter = data.begin();
    while (true)
    {
        next:
        if (iter == data.end()) break;
        auto e = *iter;
        auto ds = e->getDependees();
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
        const auto& el = *iter++;
        s << endl << "  " << el->toString();
        for (const auto& e : view(iter, state.end())) {
            s << ',' << endl << "  " << e->toString();
        }
    }
    s << endl << ')';
    return s;
}

std::ostream& operator<<(std::ostream& s, const PredicateState& state) {
    s << '(';
    if (!state.empty()) {
        auto iter = state.begin();
        const auto& el = *iter++;
        s << std::endl << "  " << el->toString();
        for (const auto& e : view(iter, state.end())) {
            s << ',' << std::endl << "  " << e->toString();
        }
    }
    s << std::endl << ')';
    return s;
}

} /* namespace borealis */

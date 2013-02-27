/*
 * PredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "Solver/ExecutionContext.h"
#include "Solver/Z3Solver.h"
#include "State/PredicateState.h"

namespace borealis {

using borealis::util::contains;
using borealis::util::view;

PredicateState::PredicateState() {}

PredicateState::PredicateState(Predicate::Ptr p) {
    data.push_back(p);
    visited.insert(p->getLocation());
}

PredicateState::PredicateState(const PredicateState& state) :
            data(state.data), visited(state.visited) {}

PredicateState::PredicateState(PredicateState&& state) :
            data(std::move(state.data)), visited(std::move(state.visited)) {}

PredicateState& PredicateState::operator=(const PredicateState& state) {
    data = state.data;
    visited = state.visited;
    return *this;
}

PredicateState& PredicateState::operator=(PredicateState&& state) {
    data = std::move(state.data);
    visited = std::move(state.visited);
    return *this;
}

PredicateState PredicateState::addPredicate(Predicate::Ptr pred) const {
    PredicateState res = PredicateState(*this);
    res.data.push_back(pred);
    res.visited.insert(pred->getLocation());
    return res;
}

PredicateState PredicateState::addAll(const PredicateState& state) const {
    if (state.isEmpty()) return *this;

    PredicateState res = PredicateState(*this);
    res.data.insert(res.data.end(), state.data.begin(), state.data.end());
    res.visited.insert(state.visited.begin(), state.visited.end());
    return res;
}

PredicateState PredicateState::addVisited(const llvm::Instruction* location) const {
    PredicateState res = PredicateState(*this);
    res.visited.insert(location);
    return res;
}

bool PredicateState::isUnreachable() const {
    z3::context ctx;
    Z3ExprFactory z3ef(ctx);
    Z3Solver s(z3ef);

    return s.checkPathPredicates(filter(PATH), filter(STATE));
}

logic::Bool PredicateState::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    TRACE_FUNC;

    ExecutionContext ctx(z3ef);

    auto res = z3ef.getTrue();
    for (auto& v : data) {
        res = res && v->toZ3(z3ef, &ctx);
    }
    res = res && ctx.toZ3();

    return res;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateState& state) {
    using borealis::util::streams::endl;
    s << '(';
    if (!state.isEmpty()) {
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
    using std::endl;
    s << '(';
    if (!state.isEmpty()) {
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

////////////////////////////////////////////////////////////////////////////////
//
// PredicateState operators
//
////////////////////////////////////////////////////////////////////////////////

const PredicateState operator&&(const PredicateState& state, Predicate::Ptr p) {
    return state.addPredicate(p);
}

const PredicateState operator+(const PredicateState& state, Predicate::Ptr p) {
    return state.addPredicate(p);
}

const PredicateState operator&&(const PredicateState& a, const PredicateState& b) {
    return a.addAll(b);
}

const PredicateState operator+(const PredicateState& a, const PredicateState& b) {
    return a.addAll(b);
}

} /* namespace borealis */

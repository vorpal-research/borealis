/*
 * BasicPredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "State/BasicPredicateState.h"

#include "Util/macros.h"

namespace borealis {

using borealis::util::contains;
using borealis::util::view;

BasicPredicateState::BasicPredicateState() :
        PredicateState(type_id<BasicPredicateState>()) {}

PredicateState::Ptr BasicPredicateState::addPredicate(Predicate::Ptr pred) const {
    ASSERTC(pred != nullptr);

    auto* res = new BasicPredicateState(*this);
    res->data.push_back(pred);
    return PredicateState::Ptr(res);
}

logic::Bool BasicPredicateState::toZ3(Z3ExprFactory& z3ef, ExecutionContext* pctx) const {
    TRACE_FUNC;

    auto res = z3ef.getTrue();
    for (auto& v : data) {
        res = res && v->toZ3(z3ef, pctx);
    }
    res = res && pctx->toZ3();

    return res;
}

PredicateState::Ptr BasicPredicateState::addVisited(const llvm::Value* loc) const {
    auto* res = new BasicPredicateState(*this);
    res->locs.insert(loc);
    return PredicateState::Ptr(res);
}

bool BasicPredicateState::hasVisited(std::initializer_list<const llvm::Value*> locs) const {
    return std::all_of(locs.begin(), locs.end(),
        [this](const llvm::Value* loc) { return contains(this->locs, loc); });
}

PredicateState::Ptr BasicPredicateState::map(Mapper m) const {
    auto* res = new BasicPredicateState();
    for (auto& p : data) res->data.push_back(m(p));
    return PredicateState::Ptr(res);
}

PredicateState::Ptr BasicPredicateState::filterByTypes(std::initializer_list<PredicateType> types) const {
    auto* res = new BasicPredicateState();
    for (auto& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&p](const PredicateType& type) { return p->getType() == type; })) {
            res->data.push_back(p);
        }
    }
    return PredicateState::Ptr(res);
}

PredicateState::Ptr BasicPredicateState::filter(Filterer f) const {
    auto* res = new BasicPredicateState();
    for (auto& p : data) if (f(p)) res->data.push_back(p);
    return PredicateState::Ptr(res);
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> BasicPredicateState::splitByTypes(std::initializer_list<PredicateType> types) const {
    auto* yes = new BasicPredicateState();
    auto* no = new BasicPredicateState();
    for (auto& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&p](const PredicateType& type) { return p->getType() == type; })) {
            yes->data.push_back(p);
        } else {
            no->data.push_back(p);
        }
    }
    return std::make_pair(PredicateState::Ptr(yes), PredicateState::Ptr(no));
}

bool BasicPredicateState::isEmpty() const {
    return data.empty();
}

std::string BasicPredicateState::toString() const {
    using std::endl;

    std::ostringstream s;
    s << '(';
    if (!this->isEmpty()) {
        auto iter = this->data.begin();
        auto& el = *iter++;
        s << endl << "  " << el->toString();
        for (auto& e : view(iter, this->data.end())) {
            s << ',' << endl << "  " << e->toString();
        }
    }
    s << endl << ')';
    return s.str();
}

} /* namespace borealis */

#include "Util/unmacros.h"

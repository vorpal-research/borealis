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
using borealis::util::head;
using borealis::util::tail;
using borealis::util::view;

BasicPredicateState::BasicPredicateState() :
        PredicateState(type_id<Self>()) {}

void BasicPredicateState::addPredicateInPlace(Predicate::Ptr pred) {
    this->data.push_back(pred);
    if (pred->getLocation()) {
        this->locs.insert(pred->getLocation());
    }
}

void BasicPredicateState::addVisitedInPlace(const llvm::Value* loc) {
    this->locs.insert(loc);
}

void BasicPredicateState::addVisitedInPlace(const Locations& locs) {
    this->locs.insert(locs.begin(), locs.end());
}

PredicateState::Ptr BasicPredicateState::addPredicate(Predicate::Ptr pred) const {
    ASSERTC(pred != nullptr);

    auto res = SelfPtr(new Self(*this));
    res->addPredicateInPlace(pred);
    return Simplified(res.release());
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
    auto res = SelfPtr(new Self(*this));
    res->addVisitedInPlace(loc);
    return Simplified(res.release());
}

bool BasicPredicateState::hasVisited(std::initializer_list<const llvm::Value*> locs) const {
    return std::all_of(locs.begin(), locs.end(),
        [this](const llvm::Value* loc) {
            return contains(this->locs, loc);
        }
    );
}

PredicateState::Ptr BasicPredicateState::map(Mapper m) const {
    auto res = SelfPtr(new Self());
    for (auto& p : data) {
        res->addPredicateInPlace(m(p));
    }
    res->addVisitedInPlace(this->locs);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::filterByTypes(std::initializer_list<PredicateType> types) const {
    auto res = SelfPtr(new Self());
    for (auto& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&p](const PredicateType& type) { return p->getType() == type; })) {
            res->addPredicateInPlace(p);
        }
    }
    res->addVisitedInPlace(this->locs);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::filter(Filterer f) const {
    auto res = SelfPtr(new Self());
    for (auto& p : data) {
        if (f(p)) {
            res->addPredicateInPlace(p);
        }
    }
    res->addVisitedInPlace(this->locs);
    return Simplified(res.release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> BasicPredicateState::splitByTypes(std::initializer_list<PredicateType> types) const {
    auto left = SelfPtr(new Self());
    auto right = SelfPtr(new Self());
    for (auto& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&p](const PredicateType& type) { return p->getType() == type; })) {
            left->addPredicateInPlace(p);
        } else {
            right->addPredicateInPlace(p);
        }
    }
    left->addVisitedInPlace(this->locs);
    right->addVisitedInPlace(this->locs);
    return std::make_pair(Simplified(left.release()), Simplified(right.release()));
}

PredicateState::Ptr BasicPredicateState::sliceOn(PredicateState::Ptr base) const {
    if (*this == *base) {
        return Simplified(new Self());
    }
    return nullptr;
}

PredicateState::Ptr BasicPredicateState::simplify() const {
    return this->shared_from_this();
}

bool BasicPredicateState::isEmpty() const {
    return data.empty();
}

std::string BasicPredicateState::toString() const {
    using std::endl;

    std::ostringstream s;
    s << '(';
    if (!this->isEmpty()) {
        s << endl << "  " << head(this->data)->toString();
        for (auto& e : tail(this->data)) {
            s << ',' << endl << "  " << e->toString();
        }
    }
    s << endl << ')';
    return s.str();
}

} /* namespace borealis */

#include "Util/unmacros.h"

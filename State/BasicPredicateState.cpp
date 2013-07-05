/*
 * BasicPredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "State/BasicPredicateState.h"
#include "Util/util.h"

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
    if (const auto* loc = pred->getLocation()) {
        this->locs.insert(loc);
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

    auto res = SelfPtr(new Self{ *this });
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
    auto res = SelfPtr(new Self{ *this });
    res->addVisitedInPlace(loc);
    return Simplified(res.release());
}

bool BasicPredicateState::hasVisited(std::initializer_list<const llvm::Value*> locs) const {
    return std::all_of(locs.begin(), locs.end(),
        [this](const llvm::Value* loc) { return contains(this->locs, loc); });
}

bool BasicPredicateState::hasVisitedFrom(std::unordered_set<const llvm::Value*>& visited) const {
    using borealis::util::containsKey;

    auto it = visited.begin();
    auto end = visited.end();
    for ( ; it != end; ) {
        if (contains(locs, *it)) {
            visited.erase(it++);
        } else {
            ++it;
        }
    }

    return visited.empty();
}

PredicateState::Ptr BasicPredicateState::map(Mapper m) const {
    auto res = SelfPtr(new Self{});
    for (auto& p : data) {
        res->addPredicateInPlace(m(p));
    }
    res->addVisitedInPlace(this->locs);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::filterByTypes(std::initializer_list<PredicateType> types) const {
    auto res = SelfPtr(new Self{});
    for (auto& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&](const PredicateType& type) { return p->getType() == type; }
        )) res->addPredicateInPlace(p);
    }
    res->addVisitedInPlace(this->locs);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::filter(Filterer f) const {
    auto res = SelfPtr(new Self{});
    for (auto& p : data) {
        if (f(p))
            res->addPredicateInPlace(p);
    }
    res->addVisitedInPlace(this->locs);
    return Simplified(res.release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> BasicPredicateState::splitByTypes(std::initializer_list<PredicateType> types) const {
    auto yes = SelfPtr(new Self{});
    auto no = SelfPtr(new Self{});
    for (auto& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&](const PredicateType& type) { return p->getType() == type; }
        )) yes->addPredicateInPlace(p);
        else no->addPredicateInPlace(p);
    }
    yes->addVisitedInPlace(this->locs);
    no->addVisitedInPlace(this->locs);
    return std::make_pair(Simplified(yes.release()), Simplified(no.release()));
}

PredicateState::Ptr BasicPredicateState::sliceOn(PredicateState::Ptr base) const {
    if (*this == *base) {
        return Simplified(new Self{});
    }
    return nullptr;
}

PredicateState::Ptr BasicPredicateState::simplify() const {
    return this->shared_from_this();
}

bool BasicPredicateState::isEmpty() const {
    return data.empty();
}

borealis::logging::logstream& BasicPredicateState::dump(borealis::logging::logstream& s) const {
    using borealis::logging::endl;
    using borealis::logging::il;
    using borealis::logging::ir;

    s << '(';
    s << il << endl;
    if (!this->isEmpty()) {
        s << head(this->data)->toString();
        for (auto& e : tail(this->data)) {
            s << ',' << endl << e->toString();
        }
    }
    s << ir << endl;
    s << ')';

    return s;
}

std::string BasicPredicateState::toString() const {
    using std::endl;

    std::ostringstream s;

    s << '(' << endl;
    if (!this->isEmpty()) {
        s << "  " << head(this->data)->toString();
        for (auto& e : tail(this->data)) {
            s << ',' << endl << "  " << e->toString();
        }
    }
    s << endl << ')';

    return s.str();
}

} /* namespace borealis */

#include "Util/unmacros.h"

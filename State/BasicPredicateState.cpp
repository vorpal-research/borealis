/*
 * BasicPredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "State/BasicPredicateState.h"

#include "Util/macros.h"

namespace borealis {

using borealis::util::contains;
using borealis::util::head;
using borealis::util::tail;
using borealis::util::view;

BasicPredicateState::BasicPredicateState() :
        PredicateState(class_tag<Self>()) {}

void BasicPredicateState::addPredicateInPlace(Predicate::Ptr pred) {
    data.push_back(pred);
}

void BasicPredicateState::addVisitedInPlace(const Locus& l) {
    locs.insert(l);
}

void BasicPredicateState::addVisitedInPlace(const Locs& ls) {
    locs.insert(ls.begin(), ls.end());
}

PredicateState::Ptr BasicPredicateState::addPredicate(Predicate::Ptr pred) const {
    ASSERTC(pred != nullptr);

    auto res = util::uniq(new Self{ *this });
    res->addPredicateInPlace(pred);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::addVisited(const Locus& loc) const {
    auto res = util::uniq(new Self{ *this });
    res->addVisitedInPlace(loc);
    return Simplified(res.release());
}

bool BasicPredicateState::hasVisited(std::initializer_list<Locus> ls) const {
    return std::all_of(ls.begin(), ls.end(),
        [this](const Locus& loc) { return contains(locs, loc); });
}

bool BasicPredicateState::hasVisitedFrom(Locs& visited) const {
    auto it = visited.begin();
    auto end = visited.end();
    for ( ; it != end ; ) {
        if (contains(locs, *it)) {
            it = visited.erase(it);
        } else {
            ++it;
        }
    }

    return visited.empty();
}

PredicateState::Locs BasicPredicateState::getVisited() const {
    return locs;
}

PredicateState::Ptr BasicPredicateState::map(Mapper m) const {
    auto res = util::uniq(new Self{});
    for (const auto& p : data) {
        res->addPredicateInPlace(m(p));
    }
    res->addVisitedInPlace(locs);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::filterByTypes(std::initializer_list<PredicateType> types) const {
    auto res = util::uniq(new Self{});
    for (const auto& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&](const PredicateType& type) { return p->getType() == type; }
        )) res->addPredicateInPlace(p);
    }
    res->addVisitedInPlace(locs);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::filter(Filterer f) const {
    auto res = util::uniq(new Self{});
    for (const auto& p : data) {
        if (f(p))
            res->addPredicateInPlace(p);
    }
    res->addVisitedInPlace(locs);
    return Simplified(res.release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> BasicPredicateState::splitByTypes(std::initializer_list<PredicateType> types) const {
    auto yes = util::uniq(new Self{});
    auto no = util::uniq(new Self{});
    for (const auto& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&](const PredicateType& type) { return p->getType() == type; }
        )) yes->addPredicateInPlace(p);
        else no->addPredicateInPlace(p);
    }
    yes->addVisitedInPlace(locs);
    no->addVisitedInPlace(locs);
    return std::make_pair(Simplified(yes.release()), Simplified(no.release()));
}

PredicateState::Ptr BasicPredicateState::sliceOn(PredicateState::Ptr on) const {
    if (*this == *on) {
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
    if (!isEmpty()) {
        s << head(data);
        for (auto& e : tail(data)) {
            s << ',' << endl << e;
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
    if (!isEmpty()) {
        s << "  " << head(data);
        for (auto& e : tail(data)) {
            s << ',' << endl << "  " << e;
        }
    }
    s << endl << ')';

    return s.str();
}

} /* namespace borealis */

#include "Util/unmacros.h"

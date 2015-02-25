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

const BasicPredicateState::Data& BasicPredicateState::getData() const {
    return data;
}

void BasicPredicateState::addPredicateInPlace(Predicate::Ptr pred) {
    data.push_back(pred);
}

void BasicPredicateState::addVisitedInPlace(const Locus& locus) {
    loci.insert(locus);
}

void BasicPredicateState::addVisitedInPlace(const Loci& loci_) {
    loci.insert(loci_.begin(), loci_.end());
}

PredicateState::Ptr BasicPredicateState::addPredicate(Predicate::Ptr pred) const {
    ASSERTC(pred != nullptr);

    auto&& res = Uniquified(*this);
    res->addPredicateInPlace(pred);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::addVisited(const Locus& locus) const {
    auto&& res = Uniquified(*this);
    res->addVisitedInPlace(locus);
    return Simplified(res.release());
}

bool BasicPredicateState::hasVisited(std::initializer_list<Locus> loci_) const {
    return std::all_of(loci_.begin(), loci_.end(),
        [&](auto&& locus) { return contains(loci, locus); }
    );
}

bool BasicPredicateState::hasVisitedFrom(Loci& visited) const {
    auto&& it = visited.begin();
    auto&& end = visited.end();
    while (it != end) {
        if (contains(loci, *it)) {
            it = visited.erase(it);
        } else {
            ++it;
        }
    }
    return visited.empty();
}

PredicateState::Loci BasicPredicateState::getVisited() const {
    return loci;
}

PredicateState::Ptr BasicPredicateState::fmap(FMapper f) const {
    return this->shared_from_this();
}

PredicateState::Ptr BasicPredicateState::map(Mapper m) const {
    auto&& res = Uniquified();
    for (auto&& p : data) {
        res->addPredicateInPlace(m(p));
    }
    res->addVisitedInPlace(loci);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::filterByTypes(std::initializer_list<PredicateType> types) const {
    auto&& res = Uniquified();
    for (auto&& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&](auto&& type) { return p->getType() == type; }
        )) res->addPredicateInPlace(p);
    }
    res->addVisitedInPlace(loci);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::filter(Filterer f) const {
    auto&& res = Uniquified();
    for (auto&& p : data) {
        if (f(p)) res->addPredicateInPlace(p);
    }
    res->addVisitedInPlace(loci);
    return Simplified(res.release());
}

PredicateState::Ptr BasicPredicateState::reverse() const {
    auto&& res = Uniquified();
    for (auto&& p : view(data.rbegin(), data.rend())) {
        res->addPredicateInPlace(p);
    }
    res->addVisitedInPlace(loci);
    return Simplified(res.release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> BasicPredicateState::splitByTypes(
        std::initializer_list<PredicateType> types) const {
    auto&& yes = Uniquified();
    auto&& no = Uniquified();
    for (auto&& p : data) {
        if (std::any_of(types.begin(), types.end(),
            [&](auto&& type) { return p->getType() == type; }
        )) yes->addPredicateInPlace(p);
        else no->addPredicateInPlace(p);
    }
    // FIXME: akhin Implement splitting also for locations
    yes->addVisitedInPlace(loci);
    no->addVisitedInPlace(loci);
    return std::make_pair(
        Simplified(yes.release()),
        Simplified(no.release())
    );
}

PredicateState::Ptr BasicPredicateState::sliceOn(PredicateState::Ptr on) const {
    // FIXME: akhin Separate empty state???
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

bool BasicPredicateState::equals(const PredicateState* other) const {
    if (auto* o = llvm::dyn_cast_or_null<Self>(other)) {
        return PredicateState::equals(other) &&
                util::equal(data, o->data,
                    [](auto&& a, auto&& b) { return *a == *b; }
                );
    } else return false;
}

borealis::logging::logstream& BasicPredicateState::dump(borealis::logging::logstream& s) const {
    using borealis::logging::endl;
    using borealis::logging::il;
    using borealis::logging::ir;

    s << "(";
    s << il << endl;
    if (not isEmpty()) {
        s << head(data);
        for (auto&& e : tail(data)) {
            s << "," << endl << e;
        }
    }
    s << ir << endl;
    s << ")";

    return s;
}

std::string BasicPredicateState::toString() const {
    using std::endl;

    std::ostringstream s;

    s << "(" << endl;
    if (not isEmpty()) {
        s << "  " << head(data);
        for (auto& e : tail(data)) {
            s << "," << endl << "  " << e;
        }
    }
    s << endl << ")";

    return s.str();
}

} /* namespace borealis */

#include "Util/unmacros.h"

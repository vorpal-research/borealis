/*
 * PredicateStateChoice.cpp
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#include "State/PredicateStateChoice.h"

#include "Util/macros.h"

namespace borealis {

using borealis::util::head;
using borealis::util::tail;

PredicateStateChoice::PredicateStateChoice(const std::vector<PredicateState::Ptr>& choices) :
        PredicateState(class_tag<Self>()),
        choices(choices) {
    ASSERTC(std::all_of(choices.begin(), choices.end(),
            [](const PredicateState::Ptr& s) { return s != nullptr; }));
};

PredicateStateChoice::PredicateStateChoice(std::vector<PredicateState::Ptr>&& choices) :
        PredicateState(class_tag<Self>()),
        choices(std::move(choices)) {
    ASSERTC(std::all_of(choices.begin(), choices.end(),
            [](const PredicateState::Ptr& s) { return s != nullptr; }));
};

PredicateState::Ptr PredicateStateChoice::addPredicate(Predicate::Ptr p) const {
    return fmap([&](PredicateState::Ptr s) { return s + p; });
}

PredicateState::Ptr PredicateStateChoice::addVisited(const llvm::Value* loc) const {
    return fmap([&](PredicateState::Ptr s) { return s << loc; });
}

bool PredicateStateChoice::hasVisited(std::initializer_list<const llvm::Value*> locs) const {
    auto visited = std::unordered_set<const llvm::Value*>(locs.begin(), locs.end());
    return hasVisitedFrom(visited);
}

bool PredicateStateChoice::hasVisitedFrom(Locs& visited) const {
    for (const auto& choice : choices) {
        if (choice->hasVisitedFrom(visited))
            return true;
    }
    return false;
}

PredicateState::Locs PredicateStateChoice::getVisited() const {
    Locs res;
    for (const auto& choice : choices) {
        auto choiceLocs = choice->getVisited();
        res.insert(choiceLocs.begin(), choiceLocs.end());
    }
    return res;
}

PredicateStateChoice::SelfPtr PredicateStateChoice::fmap_(FMapper f) const {
    std::vector<PredicateState::Ptr> mapped;
    mapped.reserve(choices.size());
    std::transform(choices.begin(), choices.end(), std::back_inserter(mapped),
        [&](const PredicateState::Ptr& choice) { return f(choice); });
    return SelfPtr(new Self{ mapped });
}

PredicateState::Ptr PredicateStateChoice::fmap(FMapper f) const {
    return Simplified(fmap_(f).release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> PredicateStateChoice::splitByTypes(std::initializer_list<PredicateType> types) const {
    std::vector<PredicateState::Ptr> yes;
    std::vector<PredicateState::Ptr> no;
    yes.reserve(choices.size());
    no.reserve(choices.size());

    for (const auto& choice: choices) {
        auto split = choice->splitByTypes(types);
        yes.push_back(split.first);
        no.push_back(split.second);
    }

    return std::make_pair(
        Simplified(new Self{ yes }),
        Simplified(new Self{ no })
    );
}

PredicateState::Ptr PredicateStateChoice::sliceOn(PredicateState::Ptr base) const {
    std::vector<PredicateState::Ptr> slices;
    slices.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(slices),
        [&](const PredicateState::Ptr& choice) { return choice->sliceOn(base); });

    if (std::all_of(slices.begin(), slices.end(),
        [](const PredicateState::Ptr& slice) { return slice != nullptr; }
    )) return Simplified(new Self{ slices });

    return nullptr;
}

PredicateState::Ptr PredicateStateChoice::simplify() const {
    std::vector<PredicateState::Ptr> simplified;
    simplified.reserve(choices.size());

    std::transform(choices.begin(), choices.end(), std::back_inserter(simplified),
        [](const PredicateState::Ptr& choice) { return choice->simplify(); });

    auto begin = simplified.begin();
    auto end = simplified.end();
    simplified.erase(
        std::remove_if(begin, end,
            [](const PredicateState::Ptr& choice) { return choice->isEmpty(); }),
        end
    );

    // TODO akhin Do smth if simplified.size() == 0

    if (simplified.size() == 1) {
        return head(simplified);
    } else {
        return PredicateState::Ptr(new Self{ simplified });
    }
}

bool PredicateStateChoice::isEmpty() const {
    return std::all_of(choices.begin(), choices.end(),
        [](const PredicateState::Ptr& choice) { return choice->isEmpty(); });
}

borealis::logging::logstream& PredicateStateChoice::dump(borealis::logging::logstream& s) const {
    using borealis::logging::endl;
    using borealis::logging::il;
    using borealis::logging::ir;

    s << "(BEGIN";
    s << il << endl;
    if (!choices.empty()) {
        s << "<OR>" << head(choices);
        for (const auto& choice : tail(choices)) {
            s << "," << endl << "<OR>" << choice;
        }
    }
    s << ir << endl;
    s << "END)";

    return s;
}

std::string PredicateStateChoice::toString() const {
    using std::endl;

    std::ostringstream s;

    s << "(BEGIN" << endl;
    if (!choices.empty()) {
        s << "<OR>" << head(choices);
        for (const auto& choice : tail(choices)) {
            s << "," << endl << "<OR>" << choice;
        }
    }
    s << endl << "END)";

    return s.str();
}

} /* namespace borealis */

#include "Util/unmacros.h"

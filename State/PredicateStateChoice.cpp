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
using borealis::util::viewContainer;

PredicateStateChoice::PredicateStateChoice(const Choices& choices) :
        PredicateState(class_tag<Self>()),
        choices(choices) {
    ASSERTC(std::all_of(choices.begin(), choices.end(),
            [](auto&& s) { return s != nullptr; }));
}

PredicateStateChoice::PredicateStateChoice(Choices&& choices) :
        PredicateState(class_tag<Self>()),
        choices(std::move(choices)) {
    ASSERTC(std::all_of(choices.begin(), choices.end(),
            [](auto&& s) { return s != nullptr; }));
};

const PredicateStateChoice::Choices& PredicateStateChoice::getChoices() const {
    return choices;
}

PredicateState::Ptr PredicateStateChoice::addPredicate(Predicate::Ptr pred) const {
    return fmap([&](PredicateState::Ptr s) { return s + pred; });
}

PredicateState::Ptr PredicateStateChoice::addVisited(const Locus& locus) const {
    return fmap([&](PredicateState::Ptr s) { return s << locus; });
}

bool PredicateStateChoice::hasVisited(std::initializer_list<Locus> loci) const {
    auto&& visited = std::unordered_set<Locus>(loci.begin(), loci.end());
    return hasVisitedFrom(visited);
}

bool PredicateStateChoice::hasVisitedFrom(Loci& visited) const {
    for (auto&& choice : choices) {
        if (choice->hasVisitedFrom(visited))
            return true;
    }
    return false;
}

PredicateState::Loci PredicateStateChoice::getVisited() const {
    Loci res;
    for (auto&& choice : choices) {
        auto&& choiceLoci = choice->getVisited();
        res.insert(choiceLoci.begin(), choiceLoci.end());
    }
    return res;
}

PredicateStateChoice::SelfPtr PredicateStateChoice::fmap_(FMapper f) const {
    return Uniquified(
        viewContainer(choices)
        .map([&](auto&& choice) { return f(choice); })
        .toVector()
    );
}

PredicateState::Ptr PredicateStateChoice::fmap(FMapper f) const {
    return Simplified(fmap_(f).release());
}

std::pair<PredicateState::Ptr, PredicateState::Ptr> PredicateStateChoice::splitByTypes(
        std::initializer_list<PredicateType> types) const {
    std::vector<PredicateState::Ptr> yes;
    std::vector<PredicateState::Ptr> no;
    yes.reserve(choices.size());
    no.reserve(choices.size());

    for (auto&& choice : choices) {
        auto&& split = choice->splitByTypes(types);
        yes.push_back(split.first);
        no.push_back(split.second);
    }

    return std::make_pair(
        Simplified<Self>(yes),
        Simplified<Self>(no)
    );
}

PredicateState::Ptr PredicateStateChoice::sliceOn(PredicateState::Ptr base) const {
    auto&& slices = viewContainer(choices)
                    .map([&](auto&& choice) { return choice->sliceOn(base); })
                    .toVector();

    if (std::all_of(slices.begin(), slices.end(),
        [](auto&& slice) { return slice != nullptr; }
    )) return Simplified<Self>(slices);

    return nullptr;
}

PredicateState::Ptr PredicateStateChoice::simplify() const {
    auto&& simplified = viewContainer(choices)
                        .map([](auto&& choice) { return choice->simplify(); })
                        .memo()
                        .filter([](auto&& choice) { return not choice->isEmpty(); })
                        .toVector();

    // FIXME: akhin Do smth if simplified.size() == 0
    //              Maybe return empty state???

    if (simplified.size() == 1) {
        return head(simplified);
    } else {
        return PredicateState::Ptr(new Self{ simplified });
    }
}

bool PredicateStateChoice::isEmpty() const {
    return std::all_of(choices.begin(), choices.end(),
        [](auto&& choice) { return choice->isEmpty(); }
    );
}

unsigned int PredicateStateChoice::size() const {
    return util::viewContainer(choices)
          .map(LAM(c, c->size()))
          .reduce(0U, ops::plus);
}

bool PredicateStateChoice::equals(const PredicateState* other) const {
    if (auto* o = llvm::dyn_cast_or_null<Self>(other)) {
        return PredicateState::equals(other) &&
                util::equal(choices, o->choices,
                    [](auto&& a, auto&& b) { return *a == *b; }
                );
    } else return false;
}

borealis::logging::logstream& PredicateStateChoice::dump(borealis::logging::logstream& s) const {
    using borealis::logging::endl;
    using borealis::logging::il;
    using borealis::logging::ir;

    s << "(BEGIN";
    s << il << endl;
    if (not choices.empty()) {
        s << "<OR>" << head(choices);
        for (auto&& choice : tail(choices)) {
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
    if (not choices.empty()) {
        s << "<OR>" << head(choices);
        for (auto&& choice : tail(choices)) {
            s << "," << endl << "<OR>" << choice;
        }
    }
    s << endl << "END)";

    return s.str();
}

} /* namespace borealis */

#include "Util/unmacros.h"

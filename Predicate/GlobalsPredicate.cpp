/*
 * GlobalsPredicate.cpp
 *
 *  Created on: Mar 13, 2013
 *      Author: ice-phoenix
 */

#include "Predicate/GlobalsPredicate.h"

namespace borealis {

GlobalsPredicate::GlobalsPredicate(
        const std::vector<Term::Ptr>& globals,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc) {

    auto&& a = util::viewContainer(globals)
                .map([](auto&& g) { return g->getName(); })
                .reduce("", [](auto&& acc, auto&& e) { return acc + "," + e; });

    asString = "globals(" + a + ")";

    ops.insert(ops.end(), globals.begin(), globals.end());
}

auto GlobalsPredicate::getGlobals() const -> decltype(util::viewContainer(ops)) {
    return util::viewContainer(ops);
}

} /* namespace borealis */

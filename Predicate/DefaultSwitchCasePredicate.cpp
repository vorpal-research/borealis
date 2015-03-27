/*
 * DefaultSwitchCasePredicate.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: ice-phoenix
 */

#include <algorithm>

#include "Predicate/DefaultSwitchCasePredicate.h"

namespace borealis {

DefaultSwitchCasePredicate::DefaultSwitchCasePredicate(
        Term::Ptr cond,
        const std::vector<Term::Ptr>& cases,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc) {

    auto&& a = util::viewContainer(cases)
                .map([](auto&& c) { return c->getName(); })
                .fold(std::string{}, [](auto&& acc, auto&& e) { return acc + "|" + e; });

    asString = cond->getName() + "=not(" + a + ")";

    ops.insert(ops.end(), cond);
    ops.insert(ops.end(), cases.begin(), cases.end());
}

Term::Ptr DefaultSwitchCasePredicate::getCond() const {
    return ops[0];
}

auto DefaultSwitchCasePredicate::getCases() const -> decltype(util::viewContainer(ops)) {
    return util::viewContainer(ops).drop(1);
}

} /* namespace borealis */

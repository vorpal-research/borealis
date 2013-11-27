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
        PredicateType type) :
            Predicate(class_tag(*this), type),
            cond(cond),
            cases(cases) {

    using borealis::util::head;
    using borealis::util::tail;

    std::string a{""};

    if (!cases.empty()) {
        a = head(cases)->getName();
        for (const auto& c : tail(cases)) {
            a = a + "|" + c->getName();
        }
    }

    asString = cond->getName() + "=not(" + a + ")";
}

bool DefaultSwitchCasePredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *cond == *o->cond &&
                util::equal(cases, o->cases,
                    [](const Term::Ptr& a, const Term::Ptr& b) { return *a == *b; }
                );
    } else return false;
}

size_t DefaultSwitchCasePredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), cond, cases);
}

} /* namespace borealis */

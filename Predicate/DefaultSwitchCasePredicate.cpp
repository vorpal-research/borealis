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
        std::vector<Term::Ptr> cases,
        PredicateType type) :
            Predicate(type_id(*this), type),
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

    this->asString = this->cond->getName() + "=not(" + a + ")";
}

bool DefaultSwitchCasePredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return *this->cond == *o->cond &&
            std::equal(cases.begin(), cases.end(), o->cases.begin(),
                [](const Term::Ptr& e1, const Term::Ptr& e2) { return *e1 == *e2; }
            );
    } else return false;
}

size_t DefaultSwitchCasePredicate::hashCode() const {
    return util::hash::defaultHasher()(type, cond, cases);
}

} /* namespace borealis */

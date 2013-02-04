/*
 * DefaultSwitchCasePredicate.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: ice-phoenix
 */

#include <algorithm>

#include "Predicate/DefaultSwitchCasePredicate.h"
#include "Util/macros.h"

namespace borealis {

DefaultSwitchCasePredicate::DefaultSwitchCasePredicate(
        PredicateType type,
        Term::Ptr cond,
        std::vector<Term::Ptr> cases) :
            DefaultSwitchCasePredicate(cond, cases, nullptr, type) {};

DefaultSwitchCasePredicate::DefaultSwitchCasePredicate(
        Term::Ptr cond,
        std::vector<Term::Ptr> cases,
        SlotTracker* /* st */,
        PredicateType type) :
            Predicate(type_id(*this), type),
            cond(cond),
            cases(cases) {

    std::string a = "";
    for (const auto& c : cases) {
        a = a + c->getName() + " | ";
    }

    this->asString = this->cond->getName() + "=not(" + a + ")";
}

logic::Bool DefaultSwitchCasePredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Integer Integer;

    auto result = z3ef.getTrue();
    auto le = z3ef.getExprForTerm(*cond).to<Integer>();

    if (le.empty()) {
        BYE_BYE(logic::Bool, "Encountered switch with non-Integer condition");
    }

    for (const auto& c : cases) {
        auto re = z3ef.getExprForTerm(*c).to<Integer>();

        if (re.empty()) {
            BYE_BYE(logic::Bool, "Encountered switch with non-Integer case");
        }

        result = result && le.getUnsafe() != re.getUnsafe();
    }

    return result;
}

bool DefaultSwitchCasePredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const DefaultSwitchCasePredicate* o = llvm::dyn_cast<DefaultSwitchCasePredicate>(other)) {
        return *this->cond == *o->cond &&
            std::equal(cases.begin(), cases.end(), o->cases.begin(),
                [](const Term::Ptr& e1, const Term::Ptr& e2) { return *e1 == *e2; }
            );
    } else {
        return false;
    }
}

size_t DefaultSwitchCasePredicate::hashCode() const {
    size_t hash = 3;
    hash = 17 * hash + cond->hashCode();
    for (const auto& c : cases) hash = 17 * hash + c->hashCode();
    return hash;
}

} /* namespace borealis */

#include "Util/unmacros.h"

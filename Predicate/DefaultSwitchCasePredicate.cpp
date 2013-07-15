/*
 * DefaultSwitchCasePredicate.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: ice-phoenix
 */

#include <algorithm>

#include "Predicate/DefaultSwitchCasePredicate.h"
#include "Util/util.h"

#include "Util/macros.h"

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

logic::Bool DefaultSwitchCasePredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Integer Integer;

    auto result = z3ef.getTrue();

    auto le = cond->toZ3(z3ef, ctx).to<Integer>();
    ASSERT(!le.empty(),
           "Encountered switch with non-Integer condition");

    for (const auto& c : cases) {
        auto re = c->toZ3(z3ef, ctx).to<Integer>();
        ASSERT(!re.empty(),
               "Encountered switch with non-Integer case");

        result = result && le.getUnsafe() != re.getUnsafe();
    }

    return result;
}

bool DefaultSwitchCasePredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const Self* o = llvm::dyn_cast<Self>(other)) {
        return *this->cond == *o->cond &&
            std::equal(cases.begin(), cases.end(), o->cases.begin(),
                [](const Term::Ptr& e1, const Term::Ptr& e2) { return *e1 == *e2; }
            );
    } else {
        return false;
    }
}

size_t DefaultSwitchCasePredicate::hashCode() const {
    return util::hash::defaultHasher()(type, cond, cases);
}

} /* namespace borealis */

#include "Util/unmacros.h"

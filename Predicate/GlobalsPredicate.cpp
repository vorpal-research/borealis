/*
 * GlobalsPredicate.cpp
 *
 *  Created on: Mar 13, 2013
 *      Author: ice-phoenix
 */

#include "Predicate/GlobalsPredicate.h"
#include "Util/macros.h"

namespace borealis {

GlobalsPredicate::GlobalsPredicate(
        const std::vector<Term::Ptr>& globals,
        PredicateType type) :
            Predicate(type_id(*this), type),
            globals(globals) {

    using borealis::util::head;
    using borealis::util::tail;

    if (globals.empty()) {
        this->asString = "globals()";
        return;
    }

    std::string a = head(globals)->getName();
    for (auto& g : tail(globals)) {
        a = a + "," + g->getName();
    }
    this->asString = "globals(" + a + ")";
}

logic::Bool GlobalsPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    ASSERTC(ctx != nullptr);

    typedef Z3ExprFactory::Pointer Pointer;

    auto res = z3ef.getTrue();
    for (auto& g : globals) {
        auto ge = g->toZ3(z3ef, ctx);
        ASSERT(ge.is<Pointer>(), "Encountered non-Pointer global value: " + g->getName());
        auto gp = ge.to<Pointer>().getUnsafe();
        res = res && gp == ctx->getDistinctPtr();
    }

    return res;
}

bool GlobalsPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const GlobalsPredicate* o = llvm::dyn_cast<GlobalsPredicate>(other)) {
        return std::equal(globals.begin(), globals.end(), o->globals.begin(),
            [](const Term::Ptr& a, const Term::Ptr& b) { return *a == *b; }
        );
    } else {
        return false;
    }
}

size_t GlobalsPredicate::hashCode() const {
    return util::hash::hasher<3, 17>()(type, globals);
}

} /* namespace borealis */

#include "Util/unmacros.h"

/*
 * MallocPredicate.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/MallocPredicate.h"
#include "Config/config.h"

#include "Util/macros.h"

namespace borealis {

MallocPredicate::MallocPredicate(
        Term::Ptr lhv,
        Term::Ptr numElements,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv), numElements(numElements) {
    this->asString = this->lhv->getName() + "=malloc(" + this->numElements->getName() + ")";
}

logic::Bool MallocPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    ASSERTC(ctx != nullptr);

    typedef Z3ExprFactory::Pointer Pointer;

    auto lhve = lhv->toZ3(z3ef, ctx);

    ASSERT(lhve.is<Pointer>(),
           "Malloc produces a non-pointer");

    unsigned long long elems = 1;
    if (const ConstTerm* cnst = llvm::dyn_cast<ConstTerm>(numElements)) {
        if (llvm::ConstantInt* intCnst = llvm::dyn_cast<llvm::ConstantInt>(cnst->getConstant())) {
            elems = intCnst->getLimitedValue();
        } else ASSERT(false, "Encountered malloc with non-integer element number");
    } else if (const OpaqueIntConstantTerm* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(numElements)) {
        elems = cnst->getValue();
    } else ASSERT(false, "Encountered malloc with non-integer/non-constant element number");

    auto lhvp = lhve.to<Pointer>().getUnsafe();

    static config::ConfigEntry<bool> NullableMallocs("analysis", "nullable-mallocs");

    if(NullableMallocs.get(true)) {
        return lhvp == z3ef.getNullPtr() || lhvp == ctx->getDistinctPtr(elems);
    } else {
        return lhvp == ctx->getDistinctPtr(elems);
    }
}

bool MallocPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const MallocPredicate* o = llvm::dyn_cast<MallocPredicate>(other)) {
        return *this->lhv == *o->lhv;
    } else {
        return false;
    }
}

size_t MallocPredicate::hashCode() const {
    return util::hash::hasher<3, 17>()(type, lhv);
}

} /* namespace borealis */

#include "Util/unmacros.h"

/*
 * AllocaPredicate.cpp
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#include "Predicate/AllocaPredicate.h"
#include "Util/macros.h"

namespace borealis {

AllocaPredicate::AllocaPredicate(
        Term::Ptr lhv,
        Term::Ptr numElements,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            numElements(numElements) {
    this->asString = this->lhv->getName() + "=alloca(" + this->numElements->getName() + ")";
}

logic::Bool AllocaPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    ASSERTC(ctx != nullptr);

    typedef Z3ExprFactory::Pointer Pointer;

    auto lhve = lhv->toZ3(z3ef, ctx);

    ASSERT(lhve.is<Pointer>(),
           "Encountered alloca with non-Pointer left side");

    ASSERT(llvm::isa<ConstTerm>(numElements) || llvm::isa<OpaqueIntConstantTerm>(numElements),
           "Encountered alloca with non-constant element number");

    unsigned long long elems = 1;
    if (const ConstTerm* cnst = llvm::dyn_cast<ConstTerm>(numElements)) {
        if (llvm::ConstantInt* intCnst = llvm::dyn_cast<llvm::ConstantInt>(cnst->getConstant())) {
            elems = intCnst->getLimitedValue();
        } else ASSERT(false, "Encountered alloca with non-integer element number");
    } else if (const OpaqueIntConstantTerm* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(numElements)) {
        elems = cnst->getValue();
    } else ASSERT(false, "Encountered alloca with non-integer/non-constant element number");

    auto lhvp = lhve.to<Pointer>().getUnsafe();
    return lhvp == ctx->getDistinctPtr(elems);
}

bool AllocaPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const AllocaPredicate* o = llvm::dyn_cast<AllocaPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->numElements == *o->numElements;
    } else {
        return false;
    }
}

size_t AllocaPredicate::hashCode() const {
    return util::hash::hasher<3, 17>()(type, lhv, numElements);
}

} /* namespace borealis */

#include "Util/unmacros.h"

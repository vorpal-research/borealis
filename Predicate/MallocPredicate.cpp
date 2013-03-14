/*
 * MallocPredicate.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/MallocPredicate.h"
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

    typedef Z3ExprFactory::Pointer Pointer;

    auto lhve = lhv->toZ3(z3ef, ctx);

    ASSERT(lhve.is<Pointer>(),
           "Malloc produces a non-pointer");

    unsigned long long elems = 20;
    if(const ConstTerm* cnst = llvm::dyn_cast<ConstTerm>(numElements)) {
        if(llvm::ConstantInt* intCnst = llvm::dyn_cast<llvm::ConstantInt>(cnst->getConstant())) {
            elems = intCnst->getLimitedValue();
        }
    } else if (const OpaqueIntConstantTerm* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(numElements)) {
        elems = cnst->getValue();
    }

    auto lhvp = lhve.to<Pointer>().getUnsafe();
    if (ctx) {
        ctx->registerDistinctPtr(lhvp);
        for(auto i = 1ULL; i < elems; ++i) ctx->registerDistinctPtr(lhvp+i);
    }

    return z3ef.getTrue();
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

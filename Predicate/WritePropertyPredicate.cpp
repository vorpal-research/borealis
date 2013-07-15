/*
 * WritePropertyPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/WritePropertyPredicate.h"

#include "Util/macros.h"

namespace borealis {

WritePropertyPredicate::WritePropertyPredicate(
        Term::Ptr propName,
        Term::Ptr lhv,
        Term::Ptr rhv,
        PredicateType type) :
            Predicate(type_id(*this), type),
            propName(propName),
            lhv(lhv),
            rhv(rhv) {
    this->asString = "write(" +
            this->propName->getName() + "," +
            this->lhv->getName() + "," +
            this->rhv->getName() +
        ")";
}

logic::Bool WritePropertyPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Pointer Pointer;

    ASSERTC(ctx != nullptr);

    ASSERT(llvm::isa<ConstTerm>(propName),
           "Property write with non-constant property name");
    auto* constPropName = llvm::cast<ConstTerm>(propName);
    auto strPropName = getAsCompileTimeString(constPropName->getConstant());
    ASSERT(!strPropName.empty(),
           "Property write with unknown property name");

    auto l = lhv->toZ3(z3ef, ctx);
    auto r = rhv->toZ3(z3ef, ctx);

    ASSERT(l.is<Pointer>(),
           "Property write with a non-pointer value");

    auto lp = l.to<Pointer>().getUnsafe();

    ctx->writeProperty(strPropName.getUnsafe(), lp, r);

    return z3ef.getTrue();
}

bool WritePropertyPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const Self* o = llvm::dyn_cast<Self>(other)) {
        return *this->propName == *o->propName &&
                *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv;
    } else {
        return false;
    }
}

size_t WritePropertyPredicate::hashCode() const {
    return util::hash::defaultHasher()(type, propName, lhv, rhv);
}

} /* namespace borealis */

#include "Util/unmacros.h"

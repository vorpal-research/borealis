/*
 * Z3ExprFactory.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#include "Config/config.h"
#include "Solver/Z3ExprFactory.h"

#include "Util/macros.h"

namespace borealis {

Z3ExprFactory::Z3ExprFactory() {
    z3::config cfg;
    cfg.set(":lift-ite", 2);
    cfg.set(":ng-lift-ite", 2);
    // cfg.set(":proof-mode", 2);

    ctx = std::unique_ptr<z3::context>(new z3::context(cfg));
}

unsigned int Z3ExprFactory::pointerSize = 32;

#define BRING_FROM_Z3EXPR_FACTORY(TYPE) typedef Z3ExprFactory::TYPE TYPE;

namespace f {

BRING_FROM_Z3EXPR_FACTORY(array)
BRING_FROM_Z3EXPR_FACTORY(expr)
BRING_FROM_Z3EXPR_FACTORY(function)
BRING_FROM_Z3EXPR_FACTORY(sort)

BRING_FROM_Z3EXPR_FACTORY(Bool)
BRING_FROM_Z3EXPR_FACTORY(Integer)
BRING_FROM_Z3EXPR_FACTORY(Pointer)
BRING_FROM_Z3EXPR_FACTORY(Real)
BRING_FROM_Z3EXPR_FACTORY(MemArray)
BRING_FROM_Z3EXPR_FACTORY(Dynamic)

}

#undef BRING_FROM_Z3EXPR_FACTORY

f::Pointer Z3ExprFactory::getPtrVar(const std::string& name) {
    return Pointer::mkVar(*ctx, name);
}

f::Pointer Z3ExprFactory::getPtrConst(int ptr) {
    return Pointer::mkConst(*ctx, ptr);
}

f::Pointer Z3ExprFactory::getNullPtr() {
    return Pointer::mkConst(*ctx, 0);
}

f::Bool Z3ExprFactory::getBoolVar(const std::string& name) {
    return Bool::mkVar(*ctx, name);
}

f::Bool Z3ExprFactory::getBoolConst(bool v) {
    return Bool::mkConst(*ctx, v);
}

f::Bool Z3ExprFactory::getTrue() {
    return getBoolConst(true);
}

f::Bool Z3ExprFactory::getFalse() {
    return getBoolConst(false);
}

f::Integer Z3ExprFactory::getIntVar(const std::string& name) {
    return f::Integer::mkVar(*ctx, name);
}

f::Integer Z3ExprFactory::getFreshIntVar(const std::string& name) {
    return f::Integer::mkFreshVar(*ctx, name);
}

f::Integer Z3ExprFactory::getIntConst(int v) {
    return f::Integer::mkConst(*ctx, v);
}

f::Real Z3ExprFactory::getRealVar(const std::string& name) {
    return f::Real::mkVar(*ctx, name);
}

f::Real Z3ExprFactory::getFreshRealVar(const std::string& name) {
    return f::Real::mkFreshVar(*ctx, name);
}

f::Real Z3ExprFactory::getRealConst(int v) {
    return f::Real::mkConst(*ctx, v);
}

f::Real Z3ExprFactory::getRealConst(double v) {
    return f::Real::mkConst(*ctx, (long long int)v);
}

f::MemArray Z3ExprFactory::getNoMemoryArray() {
    static config::ConfigEntry<bool> DefaultsToUnknown("analysis", "memory-defaults-to-unknown");

    if (DefaultsToUnknown.get(false) == false) {
        return f::MemArray::mkDefault(*ctx, "mem", Byte::mkConst(*ctx, 0xff));
    } else {
        return f::MemArray::mkFree(*ctx, "mem");
    }
}

f::Pointer Z3ExprFactory::getInvalidPtr() {
    return getNullPtr();
}

f::Bool Z3ExprFactory::isInvalidPtrExpr(f::Pointer ptr) {
    return (ptr == getInvalidPtr() || ptr == getNullPtr());
}

f::Bool Z3ExprFactory::getDistinct(const std::vector<f::Pointer>& exprs) {
    return logic::distinct(*ctx, exprs);
}

f::expr Z3ExprFactory::to_expr(Z3_ast ast) {
    return z3::to_expr( *ctx, ast );
}

f::Dynamic Z3ExprFactory::getVarByTypeAndName(
        Type::Ptr type,
        const std::string& name) {
    using llvm::isa;

    if (isa<borealis::Integer>(type))
        return getIntVar(name);
    else if (isa<borealis::Float>(type))
        return getRealVar(name);
    else if (isa<borealis::Bool>(type))
        return getBoolVar(name);
    else if (isa<borealis::Pointer>(type))
        return getPtrVar(name);
    else if (isa<borealis::UnknownType>(type))
        BYE_BYE(Dynamic, "Unknown var type in Z3 conversion")
    else if (isa<borealis::TypeError>(type))
        BYE_BYE(Dynamic, "Encountered type error in Z3 conversion")

    BYE_BYE(Dynamic, "Unreachable!");
}

////////////////////////////////////////////////////////////////////////////////

void Z3ExprFactory::initialize(llvm::TargetData* TD) {
    pointerSize = TD->getPointerSizeInBits();
}

} /* namespace borealis */

#include "Util/unmacros.h"

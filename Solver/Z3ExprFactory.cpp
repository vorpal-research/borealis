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

    Z3_set_ast_print_mode(*ctx, Z3_PRINT_SMTLIB2_COMPLIANT);
}

unsigned int Z3ExprFactory::pointerSize = 32;

#define BRING_FROM_Z3EXPR_FACTORY(TYPE) typedef Z3ExprFactory::TYPE TYPE;

namespace f {

BRING_FROM_Z3EXPR_FACTORY(Bool)
BRING_FROM_Z3EXPR_FACTORY(Integer)
BRING_FROM_Z3EXPR_FACTORY(Pointer)
BRING_FROM_Z3EXPR_FACTORY(Real)
BRING_FROM_Z3EXPR_FACTORY(MemArray)
BRING_FROM_Z3EXPR_FACTORY(Dynamic)

}

#undef BRING_FROM_Z3EXPR_FACTORY

f::Pointer Z3ExprFactory::getPtrVar(const std::string& name, bool fresh) {
    return fresh ? Pointer::mkFreshVar(*ctx, name) : Pointer::mkVar(*ctx, name);
}

f::Pointer Z3ExprFactory::getPtrConst(int ptr) {
    return Pointer::mkConst(*ctx, ptr);
}

f::Pointer Z3ExprFactory::getNullPtr() {
    return Pointer::mkConst(*ctx, 0);
}

f::Bool Z3ExprFactory::getBoolVar(const std::string& name, bool fresh) {
    return fresh ? Bool::mkFreshVar(*ctx, name) : Bool::mkVar(*ctx, name);
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

f::Integer Z3ExprFactory::getIntVar(const std::string& name, bool fresh) {
    return fresh ? Integer::mkFreshVar(*ctx, name) : Integer::mkVar(*ctx, name);
}

f::Integer Z3ExprFactory::getIntConst(int v) {
    return Integer::mkConst(*ctx, v);
}

f::Real Z3ExprFactory::getRealVar(const std::string& name, bool fresh) {
    return fresh ? Real::mkFreshVar(*ctx, name) : Real::mkVar(*ctx, name);
}

f::Real Z3ExprFactory::getRealConst(int v) {
    return Real::mkConst(*ctx, v);
}

f::Real Z3ExprFactory::getRealConst(double v) {
    return Real::mkConst(*ctx, (long long int)v);
}

f::MemArray Z3ExprFactory::getNoMemoryArray() {
    static config::ConfigEntry<bool> DefaultsToUnknown("analysis", "memory-defaults-to-unknown");

    if (DefaultsToUnknown.get(false) == false) {
        return MemArray::mkDefault(*ctx, "mem", Byte::mkConst(*ctx, 0xff));
    } else {
        return MemArray::mkFree(*ctx, "mem");
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

z3::expr Z3ExprFactory::to_expr(Z3_ast ast) {
    return z3::to_expr( *ctx, ast );
}

f::Dynamic Z3ExprFactory::getVarByTypeAndName(
        Type::Ptr type,
        const std::string& name,
        bool fresh) {
    using llvm::isa;

    if (isa<borealis::Integer>(type))
        return getIntVar(name, fresh);
    else if (isa<borealis::Float>(type))
        return getRealVar(name, fresh);
    else if (isa<borealis::Bool>(type))
        return getBoolVar(name, fresh);
    else if (isa<borealis::Pointer>(type))
        return getPtrVar(name, fresh);
    else if (isa<borealis::UnknownType>(type))
        BYE_BYE(Dynamic, "Unknown var type in Z3 conversion");
    else if (isa<borealis::TypeError>(type))
        BYE_BYE(Dynamic, "Encountered type error in Z3 conversion");

    BYE_BYE(Dynamic, "Unreachable!");
}

////////////////////////////////////////////////////////////////////////////////

void Z3ExprFactory::initialize(llvm::TargetData* TD) {
    pointerSize = TD->getPointerSizeInBits();
}

} /* namespace borealis */

#include "Util/unmacros.h"

/*
 * Z3ExprFactory.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#include "Solver/Z3ExprFactory.h"
#include "Term/Term.h"
#include "Util/macros.h"

namespace borealis {

Z3ExprFactory::Z3ExprFactory(z3::context& ctx) : ctx(ctx) {
    // Z3_update_param_value(ctx, "MACRO_FINDER", "true");
}

unsigned int Z3ExprFactory::pointerSize = 32;

#define BRING_FROM_Z3EXPR_FACTORY(TYPE) typedef Z3ExprFactory::TYPE TYPE;

namespace f {

BRING_FROM_Z3EXPR_FACTORY(expr)
BRING_FROM_Z3EXPR_FACTORY(function)
BRING_FROM_Z3EXPR_FACTORY(array)
BRING_FROM_Z3EXPR_FACTORY(sort)

BRING_FROM_Z3EXPR_FACTORY(Pointer)
BRING_FROM_Z3EXPR_FACTORY(MemArray)
BRING_FROM_Z3EXPR_FACTORY(Integer)
BRING_FROM_Z3EXPR_FACTORY(Real)
BRING_FROM_Z3EXPR_FACTORY(Bool)
BRING_FROM_Z3EXPR_FACTORY(Dynamic)

}

#undef BRING_FROM_Z3EXPR_FACTORY

f::Pointer Z3ExprFactory::getPtr(const std::string& name) {
    return Pointer::mkVar(ctx, name);
}

f::Pointer Z3ExprFactory::getNullPtr() {
    return Pointer::mkConst(ctx, 0);
}

f::Bool Z3ExprFactory::getBoolVar(const std::string& name) {
    return Bool::mkVar(ctx, name);
}

f::Bool Z3ExprFactory::getBoolConst(bool v) {
    return Bool::mkConst(ctx, v);
}

f::Bool Z3ExprFactory::getTrue() {
    return getBoolConst(true);
}

f::Bool Z3ExprFactory::getFalse() {
    return getBoolConst(false);
}

f::Integer Z3ExprFactory::getIntVar(const std::string& name) {
    return f::Integer::mkVar(ctx, name);
}

f::Integer Z3ExprFactory::getFreshIntVar(const std::string& name) {
    return f::Integer::mkFreshVar(ctx, name);
}

f::Integer Z3ExprFactory::getIntConst(int v) {
    return f::Integer::mkConst(ctx, v);
}

f::Integer Z3ExprFactory::getIntConst(const std::string& v) {
    std::istringstream ost(v);
    unsigned long long ull;
    ost >> ull;
    return Integer::mkConst(ctx, ull);
}

f::Real Z3ExprFactory::getRealVar(const std::string& name) {
    return f::Real::mkVar(ctx, name);
}

f::Real Z3ExprFactory::getFreshRealVar(const std::string& name) {
    return f::Real::mkFreshVar(ctx, name);
}

f::Real Z3ExprFactory::getRealConst(int v) {
    return f::Real::mkConst(ctx, v);
}

f::Real Z3ExprFactory::getRealConst(double v) {
    return f::Real::mkConst(ctx, (long long int)v);
}

f::Real Z3ExprFactory::getRealConst(const std::string& v) {
    std::istringstream buf(v);
    double dbl;
    buf >> dbl;
    return getRealConst(dbl);
}

f::MemArray Z3ExprFactory::getNoMemoryArray() {
    return f::MemArray::mkDefault(ctx, "mem", Byte::mkConst(ctx, 0xff));
}

f::Pointer Z3ExprFactory::getInvalidPtr() {
    return getNullPtr();
}

f::Bool Z3ExprFactory::isInvalidPtrExpr(f::Pointer ptr) {
    return (ptr == getInvalidPtr() || ptr == getNullPtr());
}

f::Bool Z3ExprFactory::getDistinct(const std::vector<f::Pointer>& exprs) {
    return logic::distinct(ctx, exprs);
}

f::expr Z3ExprFactory::to_expr(Z3_ast ast) {
    return z3::to_expr( ctx, ast );
}

f::Dynamic Z3ExprFactory::getExprForValue(
        const llvm::Value& value,
        const std::string& name) {
    return getExprByTypeAndName(valueType(value), name);
}

f::Dynamic Z3ExprFactory::getExprByTypeAndName(
        const llvm::ValueType type,
        const std::string& name) {
    using llvm::ValueType;

    switch(type) {
    case ValueType::INT_CONST:
        return getIntConst(name);
    case ValueType::INT_VAR:
        return getIntVar(name);
    case ValueType::REAL_CONST:
        return getRealConst(name);
    case ValueType::REAL_VAR:
        return getRealVar(name);
    case ValueType::BOOL_CONST:
        return getBoolConst(name == "TRUE" || name == "true");
    case ValueType::BOOL_VAR:
        return getBoolVar(name);
    case ValueType::NULL_PTR_CONST:
        return getNullPtr();
    case ValueType::PTR_CONST:
    case ValueType::PTR_VAR:
        return getPtr(name);
    case ValueType::UNKNOWN:
        BYE_BYE(Dynamic, "Unknown value type for Z3 conversion");
    default:
        BYE_BYE(Dynamic, "Unreachable!");
    }
}

////////////////////////////////////////////////////////////////////////////////

void Z3ExprFactory::initialize(llvm::TargetData* TD) {
    pointerSize = TD->getPointerSizeInBits();
}

} /* namespace borealis */

#include "Util/unmacros.h"

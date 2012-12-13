/*
 * Z3ExprFactory.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#include "Z3ExprFactory.h"

namespace borealis {

Z3ExprFactory::Z3ExprFactory(z3::context& ctx) : ctx(ctx) {
    // Z3_update_param_value(ctx, "MACRO_FINDER", "true");
}

unsigned int Z3ExprFactory::pointerSize = 32;

#define BRING_FROM_Z3EXPR_FACTORY(TYPE) typedef Z3ExprFactory::TYPE TYPE;

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

#undef BRING_FROM_Z3EXPR_FACTORY

Pointer Z3ExprFactory::getPtr(const std::string& name) {
    return Pointer::mkVar(ctx, name);
}

Pointer Z3ExprFactory::getNullPtr() {
    return Pointer::mkConst(ctx, 0);
}

Bool Z3ExprFactory::getBoolVar(const std::string& name) {
    return Bool::mkVar(ctx, name);
}

Bool Z3ExprFactory::getBoolConst(bool v) {
    return Bool::mkConst(ctx, v);
}

Bool Z3ExprFactory::getTrue() {
    return getBoolConst(true);
}

Bool Z3ExprFactory::getFalse() {
    return getBoolConst(false);
}

Integer Z3ExprFactory::getIntVar(const std::string& name, size_t bits) {
    return Integer::mkVar(ctx, name);
}

Integer Z3ExprFactory::getFreshIntVar(const std::string& name, size_t bits) {
    return Integer::mkFreshVar(ctx, name);
}

Integer Z3ExprFactory::getIntConst(int v, size_t bits) {
    return Integer::mkConst(ctx, v);
}

Integer Z3ExprFactory::getIntConst(const std::string& v, size_t bits) {
    std::istringstream ost(v);
    unsigned long long ull;
    ost >> ull;
    return Integer::mkConst(ctx, ull);
}

// FIXME: belyaev Do smth with reals
Real Z3ExprFactory::getRealVar(const std::string& name) {
    return Real::mkVar(ctx, name);
}

Real Z3ExprFactory::getFreshRealVar(const std::string& name) {
    return Real::mkFreshVar(ctx, name);
}

Real Z3ExprFactory::getRealConst(int v) {
    return Real::mkConst(ctx, v);
}

Real Z3ExprFactory::getRealConst(double v) {
    return Real::mkConst(ctx, (long long int)v);
}

Real Z3ExprFactory::getRealConst(const std::string& v) {
    std::istringstream buf(v);
    double dbl;
    buf >> dbl;
    return getRealConst(dbl);
}

MemArray Z3ExprFactory::getNoMemoryArray() {
    return MemArray::mkDefault(ctx, "mem", Byte::mkConst(ctx, 0xff));
}


Dynamic Z3ExprFactory::getExprForTerm(const Term& term, size_t bits) {
    return getExprByTypeAndName(term.getType(), term.getName(), bits);
}

Dynamic Z3ExprFactory::getExprForValue(
        const llvm::Value& value,
        const std::string& name) {
    return getExprByTypeAndName(valueType(value), name);
}

Pointer Z3ExprFactory::getInvalidPtr() {
    return getNullPtr();
}

Bool Z3ExprFactory::isInvalidPtrExpr(Pointer ptr) {
    return (ptr == getInvalidPtr() || ptr == getNullPtr());
}

Bool Z3ExprFactory::getDistinct(const std::vector<Pointer>& exprs) {
    return logic::distinct(ctx, exprs);
}

sort Z3ExprFactory::getPtrSort() {
    return Pointer::sort(ctx);
}

expr Z3ExprFactory::to_expr(Z3_ast ast) {
    return z3::to_expr( ctx, ast );
}

Dynamic Z3ExprFactory::getExprByTypeAndName(
        const llvm::ValueType type,
        const std::string& name,
        size_t bitsize) {
    using llvm::ValueType;

    switch(type) {
    case ValueType::INT_CONST:
        return getIntConst(name, (!bitsize)?32:bitsize); // FIXME
    case ValueType::INT_VAR:
        return getIntVar(name, (!bitsize)?32:bitsize); // FIXME
    case ValueType::REAL_CONST:
        return getRealConst(name);
    case ValueType::REAL_VAR:
        return getRealVar(name);
    case ValueType::BOOL_CONST:
        return getBoolConst(name == "TRUE" || name == "true"); // FIXME
    case ValueType::BOOL_VAR:
        return getBoolVar(name);
    case ValueType::NULL_PTR_CONST:
        return getNullPtr();
    case ValueType::PTR_CONST:
    case ValueType::PTR_VAR:
        return getPtr(name);
    case ValueType::UNKNOWN:
        return util::sayonara<Dynamic>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Unknown value type for Z3 conversion");
    }
}

////////////////////////////////////////////////////////////////////////////////

void Z3ExprFactory::initialize(llvm::TargetData* TD) {
    pointerSize = TD->getPointerSizeInBits();
}

} /* namespace borealis */

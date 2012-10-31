/*
 * Z3ExprFactory.h
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#ifndef Z3EXPRFACTORY_H_
#define Z3EXPRFACTORY_H_

#include <llvm/Target/TargetData.h>

#include "z3/z3++.h"

#include "util.h"
#include "../util.h"

namespace borealis {

using util::sayonara;
using util::toString;

class Z3ExprFactory {

public:

    Z3ExprFactory(z3::context& ctx);
    ~Z3ExprFactory();

    z3::context& unwrap() {
        return ctx;
    }

////////////////////////////////////////////////////////////////////////////////

    z3::expr getPtr(const std::string& name) {
        return ctx.bv_const(name.c_str(), pointerSize);
    }

    z3::expr getNullPtr() {
        return ctx.bv_val(0, pointerSize);
    }

    z3::expr getBoolVar(const std::string& name) {
        return ctx.bool_const(name.c_str());
    }

    z3::expr getBoolConst(bool v) {
        return ctx.bool_val(v);
    }

    z3::expr getBoolConst(const std::string& v) {
        return ctx.bool_val(v.c_str());
    }

    z3::expr getIntVar(const std::string& name) {
        return ctx.int_const(name.c_str());
    }

    z3::expr getIntConst(int v) {
        return ctx.int_val(v);
    }

    z3::expr getIntConst(const std::string& v) {
        return ctx.int_val(v.c_str());
    }

    z3::expr getRealVar(const std::string& name) {
        return ctx.real_const(name.c_str());
    }

    z3::expr getRealConst(int v) {
        return ctx.real_val(v);
    }

    z3::expr getRealConst(double v) {
        return getRealConst(toString(v));
    }

    z3::expr getRealConst(const std::string& v) {
        return ctx.real_val(v.c_str());
    }

    z3::expr getExprForValue(const llvm::Value& value, const std::string& name) {
        using llvm::ValueType;

        ValueType vt = valueType(value);
        switch(vt) {
        case ValueType::INT_CONST:
            return getIntConst(name);
        case ValueType::INT_VAR:
            return getIntVar(name);
        case ValueType::REAL_CONST:
            return getRealConst(name);
        case ValueType::REAL_VAR:
            return getRealVar(name);
        case ValueType::BOOL_CONST:
            return getBoolConst(name);
        case ValueType::BOOL_VAR:
            return getBoolVar(name);
        case ValueType::NULL_PTR_CONST:
            return getNullPtr();
        case ValueType::PTR_CONST:
        case ValueType::PTR_VAR:
            return getPtr(name);
        case ValueType::UNKNOWN:
            return sayonara<z3::expr>(__FILE__, __LINE__,
                    "<Z3ExprFactory> Unknown value type for Z3 conversion");
        }
    }

////////////////////////////////////////////////////////////////////////////////

    z3::func_decl getDerefFunction(z3::sort& domain, z3::sort& range) {
        return ctx.function("*", domain, range);
    }

    z3::func_decl getGEPFunction() {
        using namespace::z3;

        sort domain[] = {ctx.bv_sort(pointerSize), ctx.int_sort()};
        return ctx.function("gep", 2, domain, ctx.bv_sort(pointerSize));
    }

////////////////////////////////////////////////////////////////////////////////

    static void initialize(llvm::TargetData* TD) {
        // pointerSize = TD->getPointerSizeInBits();
    }

private:

    z3::context& ctx;

    static const unsigned int pointerSize = 32;

};

} /* namespace borealis */

#endif /* Z3EXPRFACTORY_H_ */

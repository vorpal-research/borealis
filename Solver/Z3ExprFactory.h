/*
 * Z3ExprFactory.h
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#ifndef Z3EXPRFACTORY_H_
#define Z3EXPRFACTORY_H_

#include <llvm/Target/TargetData.h>
#include <z3/z3++.h>

#include "Term/Term.h"
#include "Util/util.h"

namespace borealis {

class Z3ExprFactory {

public:

    Z3ExprFactory(z3::context& ctx);

    z3::context& unwrap() {
        return ctx;
    }

    ////////////////////////////////////////////////////////////////////////////

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
        return getRealConst(util::toString(v));
    }

    z3::expr getRealConst(const std::string& v) {
        return ctx.real_val(v.c_str());
    }

    z3::expr getEmptyMemoryArray() {
        using z3::sort;

        sort ptr_s = getPtrSort();

        return z3::const_array(ptr_s, ctx.bv_val(0xff, 8));
    }

    z3::expr getMemoryArray(
            const std::vector<std::pair<z3::expr, z3::expr>>& contents) {
        using z3::sort;

        z3::expr arr = getEmptyMemoryArray();

        for(const auto& iv: contents) {
            arr = z3::store(arr, iv.first, iv.second);
        }

        return arr;
    }

    std::vector<z3::expr> splitBytes(z3::expr bv) {
        size_t bits = bv.get_sort().bv_size();
        std::vector<z3::expr> ret;

        for(size_t ix = 0; ix < bits; ix+=8) {
            z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+7, ix, bv));
            ret.push_back(e);
        }

        return ret;
    }

    // a hack: CopyAssignable reference to non-CopyAssignable object
    // (z3::expr is CopyConstructible, but not CopyAssignable, so no
    // accumulator-like shit is possible with it)
    struct z3ExprRef{
        std::unique_ptr<z3::expr> inner;
        z3ExprRef(z3::expr e): inner(new z3::expr(e)) {};
        z3ExprRef(const z3ExprRef& ref): inner(new z3::expr(*ref.inner)) {};

        z3ExprRef& operator=(const z3ExprRef& ref) {
            inner.reset(new z3::expr(*ref.inner));
            return *this;
        }
        z3ExprRef& operator=(z3::expr e) {
            inner.reset(new z3::expr(e));
            return *this;
        }

        z3::expr get() const { return *inner; }

        operator z3::expr() { return get(); }
    };

    z3::expr concatBytes(const std::vector<z3::expr>& bytes) {
        if(bytes.empty()) return ctx.bv_val(0,1);

        z3ExprRef head = bytes[0];
        for(size_t i = 1; i < bytes.size(); ++i) {
            head = z3::expr(ctx, Z3_mk_concat(ctx, bytes[i], head.get()));
        }

        return head.get();
    }

    z3::expr byteArrayInsert(z3::expr arr, z3::expr ix, z3::expr bv) {
        auto bytes = splitBytes(bv);
        z3ExprRef ret = arr;
        z3ExprRef ixs = ix;
        int sh = 0;

        for(z3ExprRef byte: bytes) {
            ret = z3::store(ret, ixs.get() + sh, byte);
            sh++;
        }

        return ret;
    }

    z3::expr byteArrayExtract(z3::expr arr, z3::expr ix, unsigned sz) {
        std::vector<z3::expr> ret;

        for(unsigned i = 0; i < sz; ++i) {
            ret.push_back(z3::select(arr, ix+i));
        }
        return concatBytes(ret);
    }

    z3::expr getExprForTerm(
            const Term& term) {
        return getExprByTypeAndName(term.getType(), term.getName());
    }

    z3::expr getExprForValue(
            const llvm::Value& value,
            const std::string& name) {
        return getExprByTypeAndName(valueType(value), name);
    }

private:

    z3::expr getExprByTypeAndName(
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
            return getBoolConst(name);
        case ValueType::BOOL_VAR:
            return getBoolVar(name);
        case ValueType::NULL_PTR_CONST:
            return getNullPtr();
        case ValueType::PTR_CONST:
        case ValueType::PTR_VAR:
            return getPtr(name);
        case ValueType::UNKNOWN:
            return util::sayonara<z3::expr>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                    "Unknown value type for Z3 conversion");
        }
    }

public:

    ////////////////////////////////////////////////////////////////////////////

    z3::func_decl getDerefFunction(z3::sort& domain, z3::sort& range) {
        return ctx.function("*", domain, range);
    }

    z3::func_decl getGEPFunction() {
        using namespace::z3;

        sort domain[] = {ctx.bv_sort(pointerSize), ctx.int_sort()};
        return ctx.function("gep", 2, domain, ctx.bv_sort(pointerSize));
    }

    ////////////////////////////////////////////////////////////////////////////

    z3::sort getPtrSort() {
        return ctx.bv_sort(pointerSize);
    }

    ////////////////////////////////////////////////////////////////////////////

    static void initialize(llvm::TargetData* TD) {
        pointerSize = TD->getPointerSizeInBits();
    }

private:

    z3::context& ctx;

    static unsigned int pointerSize;

};

} /* namespace borealis */

#endif /* Z3EXPRFACTORY_H_ */

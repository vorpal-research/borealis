/*
 * Z3ExprFactory.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#include "Z3ExprFactory.h"

namespace borealis {

Z3ExprFactory::Z3ExprFactory(z3::context& ctx) : ctx(ctx) {}

unsigned int Z3ExprFactory::pointerSize = 32;

typedef Z3ExprFactory::expr expr;
typedef Z3ExprFactory::function function;
typedef Z3ExprFactory::array array;
typedef Z3ExprFactory::sort sort;

expr Z3ExprFactory::getPtr(const std::string& name) {
    return ctx.bv_const(name.c_str(), pointerSize);
}

expr Z3ExprFactory::getNullPtr() {
    return ctx.bv_val(0, pointerSize);
}

expr Z3ExprFactory::getBoolVar(const std::string& name) {
    return ctx.bool_const(name.c_str());
}

expr Z3ExprFactory::getBoolConst(bool v) {
    return ctx.bool_val(v);
}

expr Z3ExprFactory::getBoolConst(const std::string& v) {
    return ctx.bool_val(v.c_str());
}

expr Z3ExprFactory::getIntVar(const std::string& name, size_t bits) {
    return ctx.bv_const(name.c_str(), bits);
}

expr Z3ExprFactory::getIntConst(int v, size_t bits) {
    return ctx.bv_val(v, bits);
}

expr Z3ExprFactory::getIntConst(const std::string& v, size_t bits) {
    return ctx.bv_val(v.c_str(), bits);
}

// FIXME: do smth with reals
expr Z3ExprFactory::getRealVar(const std::string& name) {
    return ctx.bv_const(name.c_str(), 64);
}

expr Z3ExprFactory::getRealConst(int v) {
    return ctx.bv_val(v, 64);
}

expr Z3ExprFactory::getRealConst(double v) {
    return ctx.bv_val((long long)v, 64);
}

expr Z3ExprFactory::getRealConst(const std::string& v) {
    std::istringstream buf(v);
    double dbl;
    buf >> dbl;

    return getRealConst(dbl);
}

array Z3ExprFactory::getNoMemoryArray() {
    return createFreshFunction("mem", { getPtrSort() }, ctx.bv_sort(8));
}

expr Z3ExprFactory::getNoMemoryArrayAxiom(array mem) {
    return getForAll({ getPtrSort() }, [&mem](const std::vector<z3::expr>& args){
                return mem(args[0]) == 0xff;
    });
}

std::vector<expr> Z3ExprFactory::splitBytes(expr bv) {
    size_t bits = bv.get_sort().bv_size();
    std::vector<z3::expr> ret;

    for (size_t ix = 0; ix < bits; ix+=8) {
        z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+7, ix, bv));
        ret.push_back(e);
    }

    return ret;
}

expr Z3ExprFactory::concatBytes(const std::vector<z3::expr>& bytes) {
    if (bytes.empty()) return ctx.bv_val(0,1);

    exprRef head = bytes[0];
    for (size_t i = 1; i < bytes.size(); ++i) {
        head = z3::expr(ctx, Z3_mk_concat(ctx, bytes[i], head.get()));
    }

    return head.get();
}



expr Z3ExprFactory::getForAll(
        const std::vector<z3::sort>& sorts,
        std::function<z3::expr(const std::vector<z3::expr>&)> func
    ) {
    TRACE_FUNC;

    using borealis::util::toString;
    using borealis::util::view;

    size_t numBounds = sorts.size();
    std::vector<z3::expr> args;

    for(size_t i = 0U; i < numBounds; ++i) {
        args.push_back(to_expr(Z3_mk_bound(ctx, i, sorts[i])));
    }

    auto body = func(args);

    std::vector<Z3_sort> sort_array(sorts.rbegin(), sorts.rend());

    std::vector<Z3_symbol> name_array;
    for(size_t i = 0U; i < numBounds; ++i) {
        std::string name = "forall_bound_" + toString(numBounds - i -1);
        name_array.push_back(ctx.str_symbol(name.c_str()));
    }

    auto axiom = to_expr(
            Z3_mk_forall(
                    ctx,
                    0,
                    0,
                    nullptr,
                    numBounds,
                    &sort_array[0],
                    &name_array[0],
                    body));
    return axiom;
}

function Z3ExprFactory::createFreshFunction(
        const std::string& name,
        const std::vector<z3::sort>& domain,
        z3::sort range) {

    std::vector< Z3_sort > resolve( domain.begin(), domain.end() );

    return z3::func_decl(ctx,
            Z3_mk_fresh_func_decl(
                    this->ctx,
                    name.c_str(),
                    resolve.size(),
                    &resolve[0],
                    range));
}

std::pair<array, expr> Z3ExprFactory::byteArrayInsert(array arr, expr ix, expr bv) {
    TRACE_FUNC;

    auto bytes = splitBytes(bv);

    array new_arr = createFreshFunction("mem", { arr.domain(0) }, arr.range());
    exprRef ixs = ix;

    std::vector<std::pair<z3::expr, z3::expr>> baseCases;
    for(int sh = 0; sh < bytes.size(); ++sh) {
        z3::expr shift = ixs + getIntConst(sh, pointerSize);
        auto p = std::make_pair(shift, bytes[sh]);
        baseCases.push_back( p );
    }

    // need to capture everything by val :(
    auto axiom = getForAll({ ixs.get().get_sort() },
            [this, arr, new_arr, baseCases](const std::vector<z3::expr>& vs){
        TRACE_FUNC;
        // forall x. new_arr[x] = if(x == 0) ... else arr[x]
        return new_arr(vs[0]) ==
                if_(isInvalidPtrExpr(vs[0])).
                    then_(arr(vs[0])).
                    else_(switch_(vs[0], baseCases, arr(vs[0])));
    });

    return std::make_pair(new_arr, axiom.simplify());
}

expr Z3ExprFactory::byteArrayExtract(array arr, z3::expr ix, unsigned sz) {
    std::vector<z3::expr> ret;

    for(unsigned i = 0; i < sz; ++i) {
        ret.push_back(arr(ix + getIntConst(i, pointerSize)));
    }

    return concatBytes(ret);
}

expr Z3ExprFactory::getExprForTerm(const Term& term, size_t bits) {
    return getExprByTypeAndName(term.getType(), term.getName(), bits);
}

expr Z3ExprFactory::getExprForValue(
        const llvm::Value& value,
        const std::string& name) {
    return getExprByTypeAndName(valueType(value), name);
}

expr Z3ExprFactory::getInvalidPtr() {
    return getNullPtr();
}

expr Z3ExprFactory::isInvalidPtrExpr(z3::expr ptr) {
    return (ptr == getInvalidPtr() || ptr == getNullPtr()).simplify();
}

expr Z3ExprFactory::getDistinct(const std::vector<expr>& exprs) {
    if(exprs.empty()) return getBoolConst(true);

    std::vector<Z3_ast> cast(exprs.begin(), exprs.end());
    return to_expr(Z3_mk_distinct(ctx, cast.size(), &cast[0]));
}

expr Z3ExprFactory::to_expr(Z3_ast ast) {
    return z3::to_expr( ctx, ast );
}

expr Z3ExprFactory::getExprByTypeAndName(
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

////////////////////////////////////////////////////////////////////////////

function Z3ExprFactory::getDerefFunction(sort& domain, sort& range) {
    return ctx.function("*", domain, range);
}

function Z3ExprFactory::getGEPFunction() {
    using namespace::z3;

    sort domain[] = {ctx.bv_sort(pointerSize), ctx.int_sort()};
    return ctx.function("gep", 2, domain, ctx.bv_sort(pointerSize));
}

////////////////////////////////////////////////////////////////////////////

sort Z3ExprFactory::getPtrSort() {
    return ctx.bv_sort(pointerSize);
}

////////////////////////////////////////////////////////////////////////////

void Z3ExprFactory::initialize(llvm::TargetData* TD) {
    pointerSize = TD->getPointerSizeInBits();
}

} /* namespace borealis */

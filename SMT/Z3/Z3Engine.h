//
// Created by belyaev on 6/7/16.
//

#ifndef AURORA_SANDBOX_Z3ENGINE_H_H
#define AURORA_SANDBOX_Z3ENGINE_H_H

#include "SMT/Engine.h"

#include <z3/z3++.h>

#include "SMT/Result.h"
#include "SMT/Z3/Params.h"

#include "Util/macros.h"

namespace borealis {

struct Z3Engine: SmtEngine {
public:
    using expr_t = z3::expr;
    using sort_t = z3::sort;
    using function_t = z3::func_decl;
    using pattern_t = Z3_pattern;

    using context_t = z3::context;
    using solver_t = z3::solver;

    static std::unique_ptr<context_t> init() {
        z3::config cfg;

        z3_::Params::load().apply();

        auto ctx = std::unique_ptr<z3::context>(new z3::context(cfg));

        Z3_set_ast_print_mode(*ctx, Z3_PRINT_SMTLIB2_COMPLIANT);
        return std::move(ctx);
    }

    inline static size_t hash(expr_t e) { return e.hash(); }
    inline static std::string name(expr_t e) { return e.decl().name().str(); }
    inline static std::string toString(expr_t e) { return util::toString(e);  }
    inline static std::string toSMTLib(expr_t e) { return util::toString(e);  }
    inline static bool term_equality(context_t&, expr_t e1, expr_t e2) { return z3::eq(e1, e2);  }
    inline static expr_t simplify(context_t&, expr_t e) { return e.simplify(); }

    inline static sort_t bool_sort(context_t& ctx){ return ctx.bool_sort(); }
    inline static sort_t bv_sort(context_t& ctx, size_t bitsize){ return ctx.bv_sort(bitsize); }
    inline static sort_t array_sort(context_t& ctx, sort_t index, sort_t elem){
        return ctx.array_sort(index, elem);
    }

    inline static sort_t get_sort(context_t&, expr_t e) { return e.get_sort(); }
    inline static size_t bv_size(context_t&, sort_t s) { return s.bv_size(); }
    inline static bool is_bool(context_t&, expr_t e) {
        return e.is_bool();
    }
    inline static bool is_bv(context_t&, expr_t e) {
        return e.is_bv();
    }
    inline static bool is_array(context_t&, expr_t e) {
        return e.is_array();
    }

    inline static pattern_t mkPattern(context_t& ctx, expr_t expr) {
        Z3_ast pats[] { expr };
        return Z3_mk_pattern(ctx, 1, pats);
    }

    inline static expr_t mkBound(context_t& ctx, size_t ix, sort_t sort) {
        return z3::to_expr(ctx, Z3_mk_bound(ctx, ix, sort));
    }

    inline static expr_t mkForAll(
            context_t&,
            expr_t arg,
            expr_t body
    ) {
        return z3::forall(arg, body);
    }

    inline static expr_t mkForAll(
            context_t& ctx,
            const std::vector<sort_t>& sorts,
            std::function<expr_t(const std::vector<expr_t>&)> body
    ) {
        auto numArgs = sorts.size();
        std::vector<expr_t> bounds;
        bounds.reserve(numArgs);
        for(auto i = 0U; i < numArgs; ++i) {
            bounds.push_back(mkBound(ctx, i, sorts[i]));
        }
        auto realBody = body(bounds);
        std::vector<Z3_symbol> names;
        names.reserve(numArgs);
        for (size_t i = 0U; i < numArgs; ++i) {
            std::string name = tfm::format("forall_bound_%d", numArgs - i - 1);
            names.push_back(ctx.str_symbol(name.c_str()));
        }
        std::vector<Z3_sort> sorts_raw(sorts.begin(), sorts.end());

        auto axiom = z3::to_expr(
            ctx,
            Z3_mk_forall(
                ctx,
                0,
                0,
                0,
                numArgs,
                sorts_raw.data(),
                names.data(),
                realBody
            )
        );
        return axiom;
    }

    inline static expr_t mkForAll(
            context_t& ctx,
            const std::vector<sort_t>& sorts,
            std::function<expr_t(const std::vector<expr_t>&)> body,
            std::function<std::vector<pattern_t>(const std::vector<expr_t>&)> patterngen
    ) {

        auto numArgs = sorts.size();
        std::vector<expr_t> bounds;
        bounds.reserve(numArgs);
        for(auto i = 0U; i < numArgs; ++i) {
            bounds.push_back(mkBound(ctx, i, sorts[i]));
        }
        auto realBody = body(bounds);
        std::vector<Z3_symbol> names;
        names.reserve(numArgs);
        for (size_t i = 0U; i < numArgs; ++i) {
            std::string name = tfm::format("forall_bound_%d", numArgs - i - 1);
            names.push_back(ctx.str_symbol(name.c_str()));
        }
        std::vector<Z3_sort> sorts_raw(sorts.begin(), sorts.end());

        auto patterns = patterngen(bounds);
        std::vector<Z3_pattern> patterns_raw(patterns.begin(), patterns.end());

        auto axiom = z3::to_expr(
            ctx,
            Z3_mk_forall(
                ctx,
                0,
                patterns_raw.size(),
                patterns_raw.data(),
                numArgs,
                sorts_raw.data(),
                names.data(),
                realBody
            )
        );
        return axiom;
    }

    inline static expr_t mkVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        if(fresh == freshness::fresh) return z3::to_expr(ctx, Z3_mk_fresh_const(ctx, name.c_str(), sort));
        else return ctx.constant(name.c_str(), sort);
    }
    inline static expr_t mkBoolConst(context_t& ctx, bool value) {
        return ctx.bool_val(value);
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, int64_t value){
        if(sort.is_bool()) return mkBoolConst(ctx, value);
        return z3::to_expr(ctx, Z3_mk_int64(ctx, value, sort));
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, uint64_t value){
        if(sort.is_bool()) return mkBoolConst(ctx, value);
        return z3::to_expr(ctx, Z3_mk_unsigned_int64(ctx, value, sort));
    }
    template<class Numeric,
             class = std::enable_if_t<std::is_integral<Numeric>::value && std::is_signed<Numeric>::value>
    >
    inline static std::enable_if_t<std::is_integral<Numeric>::value && std::is_signed<Numeric>::value, expr_t> mkNumericConst(context_t& ctx, sort_t sort, Numeric value){
        return mkNumericConst(ctx, sort, static_cast<int64_t>(value));
    }
    template<class Numeric,
             class = std::enable_if_t<std::is_integral<Numeric>::value && std::is_unsigned<Numeric>::value>
    >
    inline static std::enable_if_t<std::is_integral<Numeric>::value && std::is_unsigned<Numeric>::value, expr_t> mkNumericConst(context_t& ctx, sort_t sort, Numeric value){
        return mkNumericConst(ctx, sort, static_cast<uint64_t>(value));
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, const std::string& valueRep){
        if(sort.is_bv()) return ctx.bv_val(valueRep.c_str(), sort.bv_size());
        if(sort.is_bool()) return mkBoolConst(ctx, valueRep != "0" && valueRep != "false");
        return ctx.int_val(valueRep.c_str());
    }

    inline static expr_t mkArrayConst(context_t&, sort_t sort, expr_t e){
        return z3::const_array(sort, e);
    }
    inline static function_t mkFunction(context_t& ctx, const std::string& name, sort_t res, const std::vector<sort_t>& args ) {
        z3::sort_vector args_(ctx);
        for(auto&& arg: args) args_.push_back(arg);
        return ctx.function(name.c_str(), args_, res);
    }

    inline static function_t mkFreshFunction(context_t& ctx, const std::string& name, sort_t res, const std::vector<sort_t>& args ) {
        std::vector<Z3_sort> args_;
        for(auto&& arg: args) args_.push_back(arg);
        auto fd = Z3_mk_fresh_func_decl(ctx, name.c_str(), args_.size(), args_.data(), res);
        return z3::func_decl(ctx, fd);
    }

    inline static expr_t applyFunction(context_t&, function_t f, const std::vector<expr_t>& args) {
        return f(args.size(), args.data());
    }

    template<class ...Exprs>
    inline static expr_t conjunction(context_t& ctx, Exprs... es) {
        Z3_ast data[sizeof...(Exprs)] = { es... };
        return z3::to_expr(ctx, Z3_mk_and(ctx, sizeof...(Exprs), data));
    }

    template<class ExprContainer>
    inline static expr_t conjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        std::vector<Z3_ast> data(std::begin(ec), std::end(ec));
        return z3::to_expr(ctx, Z3_mk_and(ctx, data.size(), data.data()));
    }

    template<class ...Exprs>
    inline static expr_t disjunction(context_t& ctx, Exprs... es) {
        Z3_ast data[sizeof...(Exprs)] = { es... };
        return z3::to_expr(ctx, Z3_mk_or(ctx, sizeof...(Exprs), data));
    }

    template<class ExprContainer>
    inline static expr_t disjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        std::vector<Z3_ast> data(std::begin(ec), std::end(ec));
        return z3::to_expr(ctx, Z3_mk_or(ctx, data.size(), data.data()));
    }

    using Z3_capi_binop = std::add_pointer_t< Z3_ast(Z3_context, Z3_ast, Z3_ast) >;
    static Z3_capi_binop binop2z3(binOp bop) {
        static const stdarray_wrapper<Z3_capi_binop, binOp, binOp::LAST> conv = []() {
            stdarray_wrapper<Z3_capi_binop, binOp, binOp::LAST> ret;
            for(auto&& el: ret) el = [](Z3_context, Z3_ast, Z3_ast) -> Z3_ast { UNREACHABLE("Incorrect opcode"); };
            ret[binOp::CONJ]    =
                [](Z3_context ctx, Z3_ast l, Z3_ast r) -> Z3_ast {
                    Z3_ast data[2]{l, r};
                    if(Z3_L_TRUE == Z3_get_bool_value(ctx, l)) return r;
                    if(Z3_L_TRUE == Z3_get_bool_value(ctx, r)) return l;
                    return Z3_mk_and(ctx, 2, data);
                };
            ret[binOp::DISJ]    =
                [](Z3_context ctx, Z3_ast l, Z3_ast r) -> Z3_ast {
                    Z3_ast data[2]{l, r};
                    if(Z3_L_FALSE == Z3_get_bool_value(ctx, l)) return r;
                    if(Z3_L_FALSE == Z3_get_bool_value(ctx, r)) return l;
                    return Z3_mk_or(ctx, 2, data);
                };
            ret[binOp::IMPLIES] = Z3_mk_implies;
            ret[binOp::IFF]     = Z3_mk_iff;
            ret[binOp::XOR]     = Z3_mk_xor;
            ret[binOp::CONCAT]  = Z3_mk_concat;
            ret[binOp::BAND]    = Z3_mk_bvand;
            ret[binOp::BOR]     = Z3_mk_bvor;
            ret[binOp::BXOR]    = Z3_mk_bvxor;
            ret[binOp::ADD]     = Z3_mk_bvadd;
            ret[binOp::SUB]     = Z3_mk_bvsub;
            ret[binOp::SMUL]    = Z3_mk_bvmul;
            ret[binOp::SDIV]    = Z3_mk_bvsdiv;
            ret[binOp::SMOD]    = Z3_mk_bvsmod;
            ret[binOp::SREM]    = Z3_mk_bvsrem;
            ret[binOp::UMUL]    = Z3_mk_bvmul;
            ret[binOp::UDIV]    = Z3_mk_bvudiv;
            ret[binOp::UMOD]    = Z3_mk_bvurem;
            ret[binOp::UREM]    = Z3_mk_bvurem;
            ret[binOp::ASHL]    = Z3_mk_bvshl;
            ret[binOp::ASHR]    = Z3_mk_bvashr;
            ret[binOp::LSHR]    = Z3_mk_bvlshr;
            ret[binOp::EQUAL]   = Z3_mk_eq;
            ret[binOp::NEQUAL]  =
                [](Z3_context ctx, Z3_ast l, Z3_ast r) -> Z3_ast { return Z3_mk_not(ctx, Z3_mk_eq(ctx, l, r)); };
            ret[binOp::SGT]     = Z3_mk_bvsgt;
            ret[binOp::SLT]     = Z3_mk_bvslt;
            ret[binOp::SGE]     = Z3_mk_bvsge;
            ret[binOp::SLE]     = Z3_mk_bvsle;
            ret[binOp::UGT]     = Z3_mk_bvugt;
            ret[binOp::ULT]     = Z3_mk_bvult;
            ret[binOp::UGE]     = Z3_mk_bvuge;
            ret[binOp::ULE]     = Z3_mk_bvule;
            ret[binOp::LOAD]    = Z3_mk_select;
            return ret;
        }();
        return conv[bop];
    }

    inline static expr_t binop(context_t& ctx, binOp bop, expr_t lhv, expr_t rhv) {
        if((bop == binOp::XOR || bop == binOp::NEQUAL) && lhv.is_bool() && rhv.is_bool()) {
            return z3::to_expr(ctx, Z3_mk_xor(ctx, lhv, rhv));
        }
        return z3::to_expr(ctx, binop2z3(bop)(ctx, lhv, rhv));
    }

    inline static expr_t unop(context_t&, unOp op, expr_t e) {
        switch(op) {
            case SmtEngine::unOp::NEGATE:
                return !e;
            case SmtEngine::unOp::BNOT:
                return ~e;
            case SmtEngine::unOp::UMINUS:
                return -e;
            default:
                break;
        }
        UNREACHABLE("Incorrect unOp value");
    }

    inline static expr_t ite(context_t&, expr_t cond, expr_t t, expr_t f) {
        return z3::ite(cond, t, f);
    }

    inline static expr_t extract(context_t&, expr_t lhv, size_t hi, size_t lo){
        return lhv.extract(hi, lo);
    }
    inline static expr_t sext(context_t& ctx, expr_t lhv, size_t sz) {
        return z3::to_expr(ctx, Z3_mk_sign_ext(ctx, sz, lhv));
    }
    inline static expr_t zext(context_t& ctx, expr_t lhv, size_t sz) {
        return z3::to_expr(ctx, Z3_mk_zero_ext(ctx, sz, lhv));
    }
    inline static expr_t store(context_t&, expr_t arr, expr_t ix, expr_t elem){
        return z3::store(arr, ix, elem);
    }
    inline static expr_t apply(context_t& ctx, function_t f, const std::vector<expr_t>& args){
        z3::expr_vector ev(ctx);
        for(auto&& arg: args) ev.push_back(arg);
        return f(ev);
    }

    inline static expr_t distinct(context_t& ctx, const std::vector<expr_t>& args) {
        std::vector<Z3_ast> cast { args.begin(), args.end() };
        return z3::to_expr(ctx, Z3_mk_distinct(ctx, cast.size(), cast.data()));
    }

    using Result = smt::Result;

    struct equality {
         bool operator()(expr_t e1, expr_t e2) const noexcept {
             return z3::eq(e1, e2);
         }
    };
};

}

namespace std {

template<>
struct hash<z3::expr> {
    size_t operator()(z3::expr e) const noexcept {
        return static_cast<size_t>(e.hash());
    }
};

} /* namespace std */

#include "Util/unmacros.h"

#endif //AURORA_SANDBOX_Z3ENGINE_H_H

//
// Created by belyaev on 6/7/16.
//

#ifndef AURORA_SANDBOX_Z3ENGINE_H_H
#define AURORA_SANDBOX_Z3ENGINE_H_H

#include "SMT/Engine.h"

#include <z3/z3++.h>

namespace borealis {

struct Z3Engine: SmtEngine {
public:
    using expr_t = z3::expr;
    using sort_t = z3::sort;
    using function_t = z3::func_decl;

    using context_t = z3::context;
    using solver_t = z3::solver;

    inline static size_t hash(expr_t e) { return e.hash(); }
    inline static std::string name(expr_t e) { return z3::; }
    inline static std::string toString(expr_t e) { return util::toString(e);  }
    inline static bool term_equality(context_t&, expr_t e1, expr_t e2) { return z3::eq(e1, e2);  }

    inline static sort_t bool_sort(context_t& ctx){ return ctx.bool_sort(); }
    inline static sort_t bv_sort(context_t& ctx, size_t bitsize){ return ctx.bv_sort(bitsize); }
    inline static sort_t array_sort(context_t& ctx, sort_t index, sort_t elem){
        return ctx.array_sort(index, elem);
    }

    inline static expr_t mkVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        if(fresh == freshness::fresh) return z3::to_expr(ctx, Z3_mk_fresh_const(ctx, name.c_str(), sort));
        else return ctx.constant(name.c_str(), sort);
    }
    inline static expr_t mkBoolConst(context_t& ctx, bool value) {
        return ctx.bool_val(value);
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, int64_t value){
        if(sort.is_bv()) return ctx.bv_val(value, sort.bv_size());
        if(sort.is_bool()) return mkBoolConst(ctx, value);
        return ctx.int_val(value);
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, uint64_t value){
        if(sort.is_bv()) return ctx.bv_val(value, sort.bv_size());
        if(sort.is_bool()) return mkBoolConst(ctx, value);
        return ctx.int_val(value);
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, const std::string& valueRep){
        if(sort.is_bv()) return ctx.bv_val(valueRep.c_str(), sort.bv_size());
        if(sort.is_bool()) return mkBoolConst(ctx, valueRep != "0" && valueRep != "false");
        return ctx.int_val(valueRep.c_str());
    }

    inline static expr_t mkArrayConst(context_t& ctx, sort_t sort, expr_t e){
        return ctx.array_sort(sort, e);
    }
    inline static function_t mkFunction(context_t& ctx, const std::string& name, sort_t res, const std::vector<sort_t>& args ) {
        z3::sort_vector args_;
        for(auto&& arg: args) args_.push_back(arg);
        return ctx.function(name.c_str(), args_, res);
    }

    using Z3_capi_binop = std::add_pointer_t< Z3_ast(Z3_context, Z3_ast, Z3_ast) >;
    static Z3_capi_binop binop2z3(binOp bop) {
        static const std::array<binOp::LAST, Z3_capi_binop> conv = []() {
            std::array<binOp::LAST, Z3_capi_binop> ret;
            for(auto&& el: ret) el = [](Z3_context, Z3_ast, Z3_ast) -> Z3_ast { UNREACHABLE("Incorrect opcode"); };
            ret[binOp::CONJ]    = Z3_mk_bvand;
            ret[binOp::DISJ]    = Z3_mk_bvor;
            ret[binOp::IMPLIES] = Z3_mk_implies;
            ret[binOp::IFF]     = Z3_mk_iff;
            ret[binOp::CONCAT]  = Z3_mk_concat;
            ret[binOp::BAND]    = Z3_mk_bvand;
            ret[binOp::BOR]     = Z3_mk_bvor;
            ret[binOp::XOR]     = Z3_mk_bvxor;
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
            ret[binOp::LOAD]    = Z3_mk_select;
            return ret;
        };
        return conv[bop];
    }

    inline static expr_t binop(context_t& ctx, binOp bop, expr_t lhv, expr_t rhv) {
        return z3::to_expr(ctx, binop2z3(bop)(ctx, lhv, rhv));
    }
    inline static expr_t unop(context_t&, UnOp, expr_t) { return {}; }

    inline static expr_t ite(context_t&, expr_t cond, expr_t t, expr_t f) {
        return z3::ite(cond, t, f);
    }

    inline static expr_t extract(context_t&, expr_t lhv, size_t hi, size_t lo){
        return lhv.extract(hi, lo);
    }
    inline static expr_t sext(context_t& ctx, expr_t lhv, size_t sz) {
        return z3::to_expr(ctx, Z3_mk_sign_ext(ctx, lhv, sz));
    }
    inline static expr_t zext(context_t& ctx, expr_t lhv, size_t sz) {
        return z3::to_expr(ctx, Z3_mk_zero_ext(ctx, lhv, sz));
    }
    inline static expr_t store(context_t&, expr_t arr, expr_t ix, expr_t elem){
        return z3::store(arr, ix, elem);
    }
    inline static expr_t apply(context_t&, function_t f, const std::vector<expr_t>& args){
        z3::expr_vector ev;
        for(auto&& arg: args) ev.push_back(arg);
        return f(ev);
    }
};

}

#endif //AURORA_SANDBOX_Z3ENGINE_H_H

//
// Created by belyaev on 6/7/16.
//

#ifndef MATHSATENGINE_H_
#define MATHSATENGINE_H_

#include "SMT/Engine.h"

#include "SMT/MathSAT/MathSAT.h"

#include "SMT/Result.h"

#include "Util/macros.h"

namespace borealis {

struct MathSATEngine: SmtEngine {
public:
    using expr_t = mathsat::Expr;
    using sort_t = mathsat::Sort;
    using function_t = mathsat::Decl;
    using pattern_t = int;

    using context_t = mathsat::Env;
    using solver_t = mathsat::Solver;

    inline static size_t hash(expr_t e) { return e.get_id(); }
    inline static std::string name(expr_t e) { return e.decl().name(); }
    inline static std::string toString(expr_t e) { return util::toString(e);  }
    inline static std::string toSMTLib(expr_t e) { return util::toString(e);  }
    inline static bool term_equality(context_t&, expr_t e1, expr_t e2) { return e1.get_id() == e2.get_id();  }
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

    inline static pattern_t mkPattern(context_t&, expr_t) {
        UNREACHABLE("patterns not supported by MathSAT")
    }

    inline static expr_t mkBound(context_t&, size_t, sort_t) {
        UNREACHABLE("bounds not supported by MathSAT")
    }

    inline static expr_t mkForAll(context_t&, expr_t, expr_t) {
        UNREACHABLE("quantifiers not supported by MathSAT")
    }

    inline static expr_t mkForAll(
            context_t&,
            const std::vector<sort_t>&,
            std::function<expr_t(const std::vector<expr_t>&)>
    ) {
        UNREACHABLE("quantifiers not supported by MathSAT")
    }

    inline static expr_t mkForAll(
            context_t&,
            const std::vector<sort_t>&,
            std::function<expr_t(const std::vector<expr_t>&)>,
            std::function<std::vector<pattern_t>(const std::vector<expr_t>&)>
    ) {
        UNREACHABLE("quantifiers not supported by MathSAT")
    }

    inline static expr_t mkVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        if(fresh == freshness::fresh) return ctx.fresh_constant(name, sort);
        else return ctx.constant(name, sort);
    }
    inline static expr_t mkBoolConst(context_t& ctx, bool value) {
        return ctx.bool_val(value);
    }

    template<class Numeric,
             class = std::enable_if_t<std::is_integral<Numeric>::value>>
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, Numeric value){
        if(sort.is_bool()) return mkBoolConst(ctx, !!value);
        return ctx.bv_val(util::toString(static_cast<unsigned long long>(value)), sort.bv_size());
    }

    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, const std::string& valueRep){
        if(sort.is_bv()) return ctx.bv_val(valueRep.c_str(), sort.bv_size());
        if(sort.is_bool()) return mkBoolConst(ctx, valueRep != "0" && valueRep != "false");
        UNREACHABLE("unknown sort encountered by MathSAT backend")
    }

    inline static expr_t mkArrayConst(context_t&, sort_t, expr_t){
        UNREACHABLE("arrays not supported by MathSAT backend")
    }
    inline static function_t mkFunction(context_t& ctx, const std::string& name, sort_t res, const std::vector<sort_t>& args ) {
        return ctx.function(name, args, res);
    }

    inline static function_t mkFreshFunction(context_t& ctx, const std::string& name, sort_t res, const std::vector<sort_t>& args ) {
        return ctx.fresh_function(name, args, res);
    }

    inline static expr_t applyFunction(context_t&, function_t f, const std::vector<expr_t>& args) {
        return f(args);
    }

    inline static expr_t conjunction(context_t& ctx) {
        return ctx.bool_val(true);
    }
    template<class HExpr, class ...TExprs>
    inline static expr_t conjunction(context_t& ctx, HExpr he, TExprs... tes) {
        return he && conjunction(ctx, tes...);
    }

    template<class ExprContainer>
    inline static expr_t conjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        if(util::viewContainer(ec).size() == 0) return ctx.bool_val(true);
        return util::head(ec) && conjunctionOfCollection(ctx, util::tail(ec));
    }


    inline static expr_t disjunction(context_t& ctx) {
        return ctx.bool_val(false);
    }
    template<class HExpr, class ...TExprs>
    inline static expr_t disjunction(context_t& ctx, HExpr h, TExprs... t) {
        return h || disjunction(ctx, t...);
    }

    template<class ExprContainer>
    inline static expr_t disjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        if(util::viewContainer(ec).size() == 0) return ctx.bool_val(false);
        return util::head(ec) || disjunctionOfCollection(ctx, util::tail(ec));
    }

    using msat_capi_binop = std::add_pointer_t< expr_t(expr_t, expr_t) >;
    static msat_capi_binop binop2f(binOp bop) {
        static const stdarray_wrapper<msat_capi_binop, binOp, binOp::LAST> conv = []() {
            stdarray_wrapper<msat_capi_binop, binOp, binOp::LAST> ret;
            for(auto&& el: ret) el = [](expr_t, expr_t) -> expr_t { UNREACHABLE("Incorrect opcode"); };
            ret[binOp::CONJ]    =
                [](expr_t l, expr_t r) -> expr_t { return l && r; };
            ret[binOp::DISJ]    =
                [](expr_t l, expr_t r) -> expr_t { return l || r; };
            ret[binOp::IMPLIES] =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::implies(l, r); };
            ret[binOp::IFF]     =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::iff(l, r); };
            ret[binOp::CONCAT]  =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::concat(l, r); };
            ret[binOp::BAND]    =
                [](expr_t l, expr_t r) -> expr_t { return l & r; };
            ret[binOp::BOR]     =
                [](expr_t l, expr_t r) -> expr_t { return l | r; };
            ret[binOp::XOR]     =
                [](expr_t l, expr_t r) -> expr_t { return l ^ r; };
            ret[binOp::ADD]     =
                [](expr_t l, expr_t r) -> expr_t { return l + r; };
            ret[binOp::SUB]     =
                [](expr_t l, expr_t r) -> expr_t { return l - r; };
            ret[binOp::SMUL]    =
                [](expr_t l, expr_t r) -> expr_t { return l * r; };
            ret[binOp::SDIV]    =
                [](expr_t l, expr_t r) -> expr_t { return l / r; };
            ret[binOp::SMOD]    =
                [](expr_t l, expr_t r) -> expr_t { return l % r; };
            ret[binOp::SREM]    =
                [](expr_t l, expr_t r) -> expr_t { return l % r; };
            ret[binOp::UMUL]    =
                [](expr_t l, expr_t r) -> expr_t { return l * r; };
            ret[binOp::UDIV]    =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::udiv(l, r); };
            ret[binOp::UMOD]    =
                [](expr_t l, expr_t r) -> expr_t { return l % r; };
            ret[binOp::UREM]    =
                [](expr_t l, expr_t r) -> expr_t { return l % r; };
            ret[binOp::ASHL]    =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::lshl(l, r); };
            ret[binOp::ASHR]    =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::ashr(l, r); };
            ret[binOp::LSHR]    =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::lshr(l, r); };
            ret[binOp::EQUAL]   =
                [](expr_t l, expr_t r) -> expr_t { return l == r; };
            ret[binOp::NEQUAL]  =
                [](expr_t l, expr_t r) -> expr_t { return l != r; };
            ret[binOp::SGT]     =
                [](expr_t l, expr_t r) -> expr_t { return l > r; };
            ret[binOp::SLT]     =
                [](expr_t l, expr_t r) -> expr_t { return l < r; };
            ret[binOp::SGE]     =
                [](expr_t l, expr_t r) -> expr_t { return l >= r; };
            ret[binOp::SLE]     =
                [](expr_t l, expr_t r) -> expr_t { return l <= r; };
            ret[binOp::UGT]     =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::ugt(l, r); };
            ret[binOp::ULT]     =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::ult(l, r); };
            ret[binOp::UGE]     =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::uge(l, r); };
            ret[binOp::ULE]     =
                [](expr_t l, expr_t r) -> expr_t { return mathsat::ule(l, r); };
            ret[binOp::LOAD]    =
                [](expr_t, expr_t) -> expr_t { UNREACHABLE("arrays not supported by mathsat backend") };
            return ret;
        }();
        return conv[bop];
    }

    inline static expr_t binop(context_t&, binOp bop, expr_t lhv, expr_t rhv) {
        return binop2f(bop)(lhv, rhv);
    }

    inline static expr_t unop(context_t&, unOp op, expr_t e) {
        switch(op) {
            case SmtEngine::unOp::NEGATE:
                return !e;
            case SmtEngine::unOp::BNOT:
                return ~e;
            case SmtEngine::unOp::UMINUS:
                return -e;
            default: break;
        }
        UNREACHABLE("Incorrect unOp value");
    }

    inline static expr_t ite(context_t&, expr_t cond, expr_t t, expr_t f) {
        return mathsat::ite(cond, t, f);
    }

    inline static expr_t extract(context_t&, expr_t lhv, size_t hi, size_t lo){
        return mathsat::extract(lhv, hi, lo);
    }
    inline static expr_t sext(context_t&, expr_t lhv, size_t sz) {
        return mathsat::sext(lhv, sz);
    }
    inline static expr_t zext(context_t&, expr_t lhv, size_t sz) {
        return mathsat::zext(lhv, sz);
    }
    inline static expr_t store(context_t&, expr_t, expr_t, expr_t){
        UNREACHABLE("arrays not supported by MathSAT backend")
    }
    inline static expr_t apply(context_t&, function_t f, const std::vector<expr_t>& args){
        return f(args);
    }

    inline static expr_t distinct(context_t&, const std::vector<expr_t>& args) {
        return mathsat::distinct(args);
    }

    using Result = smt::Result;
};

}

namespace std {

template<>
struct hash<borealis::MathSATEngine::expr_t> {
    size_t operator()(borealis::MathSATEngine::expr_t e) const noexcept {
        return static_cast<size_t>(borealis::MathSATEngine::hash(e));
    }
};

} /* namespace std */

#include "Util/unmacros.h"

#endif //MATHSATENGINE_H_

//
// Created by belyaev on 6/15/16.
//

#ifndef CVC4ENGINE_H_H
#define CVC4ENGINE_H_H

#include "SMT/Engine.h"

#include "SMT/CVC4/cvc4_fixed.h"
#include "SMT/Result.h"

#include "Util/macros.h"

namespace borealis {

struct CVC4Engine: SmtEngine {
public:
    using expr_t = CVC4::Expr;
    using sort_t = CVC4::Type;
    using function_t = CVC4::Expr;
    using pattern_t = int;

    struct context_t {
        ::CVC4::ExprManager em;
        ::CVC4::SymbolTable st;
    };
    using solver_t = CVC4::SmtEngine;

    static std::unique_ptr<context_t> init() {
        return util::uniq(new context_t());
    }

    inline static size_t hash(expr_t e) { return e.getId(); }
    inline static std::string name(expr_t e) { return e.toString(); }
    inline static std::string toString(expr_t e) { return util::toString(e);  }
    inline static std::string toSMTLib(expr_t e) { return util::toString(e);  }
    inline static bool term_equality(context_t&, expr_t e1, expr_t e2) { return e1.getId() == e2.getId();  }
    inline static expr_t simplify(context_t&, expr_t e) { return e; }

    inline static sort_t bool_sort(context_t& ctx){ return ctx.em.booleanType(); }
    inline static sort_t bv_sort(context_t& ctx, size_t bitsize){ return ctx.em.mkBitVectorType(bitsize); }
    inline static sort_t array_sort(context_t& ctx, sort_t index, sort_t elem){
        return ctx.em.mkArrayType(index, elem);
    }

    inline static sort_t get_sort(context_t&, expr_t e) { return e.getType(false); }
    inline static size_t bv_size(context_t&, sort_t s) { return CVC4::BitVectorType(s).getSize(); }
    inline static bool is_bool(context_t&, expr_t e) {
        return e.getType(false).isBoolean();
    }
    inline static bool is_bv(context_t&, expr_t e) {
        return e.getType(false).isBitVector();
    }
    inline static bool is_array(context_t&, expr_t e) {
        return e.getType(false).isArray();
    }

    inline static pattern_t mkPattern(context_t&, expr_t) {
        UNREACHABLE("Patterns not supported by CVC4 backend")
    }

    inline static expr_t mkBound(context_t&, size_t, sort_t) {
        UNREACHABLE("Bounds not supported by CVC4 backend")
    }

    inline static expr_t mkForAll(
        context_t&,
        expr_t,
        expr_t
    ) {
        UNREACHABLE("Quantifiers not supported by CVC4 backend")
    }

    inline static expr_t mkForAll(
        context_t&,
        const std::vector<sort_t>&,
        std::function<expr_t(const std::vector<expr_t>&)>
    ) {
        UNREACHABLE("Quantifiers not supported by CVC4 backend")
    }

    inline static expr_t mkForAll(
        context_t&,
        const std::vector<sort_t>&,
        std::function<expr_t(const std::vector<expr_t>&)>,
        std::function<std::vector<pattern_t>(const std::vector<expr_t>&)>
    ) {
        UNREACHABLE("Quantifiers not supported by CVC4 backend")
    }

    inline static expr_t mkVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        if (fresh == freshness::fresh) return ctx.em.mkVar(name, sort);

        if(ctx.st.isBound(name)) {
            return ctx.st.lookup(name);
        }
        auto ret = ctx.em.mkVar(name, sort);
        ctx.st.bind(name, ret);
        return ret;
    }
    inline static expr_t mkBoolConst(context_t& ctx, bool value) {
        return ctx.em.mkConst(value);
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, unsigned long value){
        if(sort.isBoolean()) return ctx.em.mkConst(!!value);
        else {
            return ctx.em.mkConst(CVC4::BitVector(bv_size(ctx, sort), value));
        }
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, long value){
        if(sort.isBoolean()) return ctx.em.mkConst(!!value);

        if(value > 0) {
            return mkNumericConst(ctx, sort, static_cast<unsigned long long>(value));
        }
        else {
            auto res = mkNumericConst(ctx, sort, static_cast<unsigned long long>(-value));
            return ctx.em.mkExpr(CVC4::kind::BITVECTOR_NEG, res);
        }
    }
    template<class Numeric,
        class = std::enable_if_t<std::is_integral<Numeric>::value && std::is_signed<Numeric>::value>
    >
    inline static std::enable_if_t<std::is_integral<Numeric>::value && std::is_signed<Numeric>::value, expr_t> mkNumericConst(context_t& ctx, sort_t sort, Numeric value){
        return mkNumericConst(ctx, sort, static_cast<long>(value));
    }
    template<class Numeric,
        class = std::enable_if_t<std::is_integral<Numeric>::value && std::is_unsigned<Numeric>::value>
    >
    inline static std::enable_if_t<std::is_integral<Numeric>::value && std::is_unsigned<Numeric>::value, expr_t> mkNumericConst(context_t& ctx, sort_t sort, Numeric value){
        return mkNumericConst(ctx, sort, static_cast<unsigned long>(value));
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, const std::string& valueRep){
        if(sort.isBoolean()) return mkBoolConst(ctx, valueRep != "0" && valueRep != "false");
        if(sort.isBitVector()) return ctx.em.mkConst(CVC4::BitVector(CVC4::BitVectorType(sort).getSize(), CVC4::Integer(valueRep, 10)));
        UNREACHABLE("Unsupported sort encountered");
    }

    inline static expr_t mkArrayConst(context_t& ctx, sort_t sort, expr_t e){
        return ctx.em.mkConst(CVC4::ArrayStoreAll(sort, e));
    }
    inline static function_t mkFunction(context_t& ctx, const std::string& name, sort_t res, const std::vector<sort_t>& args ) {
        auto ftype = ctx.em.mkFunctionType(args, res);
        return mkVar(ctx, ftype, name, freshness::normal);
    }

    inline static function_t mkFreshFunction(context_t& ctx, const std::string& name, sort_t res, const std::vector<sort_t>& args ) {
        auto ftype = ctx.em.mkFunctionType(args, res);
        return mkVar(ctx, ftype, name, freshness::fresh);
    }

    inline static expr_t applyFunction(context_t& ctx, function_t f, const std::vector<expr_t>& args) {
        return ctx.em.mkExpr(CVC4::kind::APPLY_UF, f, args);
    }

    template<class ...Exprs>
    inline static expr_t conjunction(context_t& ctx, Exprs... es) {
        return ctx.em.mkExpr(CVC4::kind::AND, es...);
    }

    template<class ExprContainer>
    inline static expr_t conjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        std::vector<expr_t> args(std::begin(ec), std::end(ec));
        return ctx.em.mkExpr(CVC4::kind::AND, args);
    }

    template<class ...Exprs>
    inline static expr_t disjunction(context_t& ctx, Exprs... es) {
        return ctx.em.mkExpr(CVC4::kind::OR, es...);
    }

    template<class ExprContainer>
    inline static expr_t disjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        std::vector<expr_t> args(std::begin(ec), std::end(ec));
        return ctx.em.mkExpr(CVC4::kind::OR, args);
    }

    using cvc4_opgen = std::add_pointer_t< CVC4::kind::Kind_t() >;
    static cvc4_opgen binop2cvc4(binOp bop) {
        static const stdarray_wrapper<cvc4_opgen, binOp, binOp::LAST> conv = []() {
            using namespace CVC4;
            stdarray_wrapper<cvc4_opgen, binOp, binOp::LAST> ret;
            for(auto&& el: ret) el = []() -> CVC4::kind::Kind_t { UNREACHABLE("Incorrect opcode"); };
            ret[binOp::CONJ]    = []() { return kind::AND; };
            ret[binOp::DISJ]    = []() { return kind::OR; };
            ret[binOp::IMPLIES] = []() { return kind::IMPLIES; };
            ret[binOp::IFF]     = []() { return kind::EQUAL; };
            ret[binOp::XOR]     = []() { return kind::XOR; };
            ret[binOp::CONCAT]  = []() { return kind::BITVECTOR_CONCAT; };
            ret[binOp::BAND]    = []() { return kind::BITVECTOR_AND; };
            ret[binOp::BOR]     = []() { return kind::BITVECTOR_OR; };
            ret[binOp::BXOR]    = []() { return kind::BITVECTOR_XOR; };
            ret[binOp::ADD]     = []() { return kind::BITVECTOR_PLUS; };
            ret[binOp::SUB]     = []() { return kind::BITVECTOR_SUB; };
            ret[binOp::SMUL]    = []() { return kind::BITVECTOR_MULT; };
            ret[binOp::SDIV]    = []() { return kind::BITVECTOR_SDIV; };
            ret[binOp::SMOD]    = []() { return kind::BITVECTOR_SMOD; };
            ret[binOp::SREM]    = []() { return kind::BITVECTOR_SREM; };
            ret[binOp::UMUL]    = []() { return kind::BITVECTOR_MULT; };
            ret[binOp::UDIV]    = []() { return kind::BITVECTOR_UDIV; };
            ret[binOp::UMOD]    = []() { return kind::BITVECTOR_UREM; };
            ret[binOp::UREM]    = []() { return kind::BITVECTOR_UREM; };
            ret[binOp::ASHL]    = []() { return kind::BITVECTOR_SHL;  };
            ret[binOp::ASHR]    = []() { return kind::BITVECTOR_ASHR; };
            ret[binOp::LSHR]    = []() { return kind::BITVECTOR_LSHR; };
            ret[binOp::EQUAL]   = []() { return kind::EQUAL; };
            // NEQUAL not handled
            ret[binOp::SGT]     = []() { return kind::BITVECTOR_SGT; };
            ret[binOp::SLT]     = []() { return kind::BITVECTOR_SLT; };
            ret[binOp::SGE]     = []() { return kind::BITVECTOR_SGE; };
            ret[binOp::SLE]     = []() { return kind::BITVECTOR_SLE; };
            ret[binOp::UGT]     = []() { return kind::BITVECTOR_UGT; };
            ret[binOp::ULT]     = []() { return kind::BITVECTOR_ULT; };
            ret[binOp::UGE]     = []() { return kind::BITVECTOR_UGE; };
            ret[binOp::ULE]     = []() { return kind::BITVECTOR_ULE; };
            ret[binOp::LOAD]    = []() { return kind::SELECT; };
            return ret;
        }();
        return conv[bop];
    }

    inline static expr_t binop(context_t& ctx, binOp bop, expr_t lhv, expr_t rhv) {
        using namespace CVC4;
        if(lhv.getType().isBoolean() && rhv.getType().isBoolean()) {
            if(bop == binOp::EQUAL) return ctx.em.mkExpr(kind::EQUAL, lhv, rhv);
            if(bop == binOp::NEQUAL) return ctx.em.mkExpr(kind::XOR, lhv, rhv);
            if(bop == binOp::XOR) return ctx.em.mkExpr(kind::XOR, lhv, rhv);
        }
        if(bop == binOp::NEQUAL) {
            return ctx.em.mkExpr(kind::NOT, binop(ctx, binOp::EQUAL, lhv, rhv));
        }
        return ctx.em.mkExpr(binop2cvc4(bop)(), lhv, rhv);
    }

    inline static expr_t unop(context_t& ctx, unOp op, expr_t e) {
        using namespace CVC4;
        switch(op) {
            case borealis::SmtEngine::unOp::NEGATE:
                return ctx.em.mkExpr(kind::NOT, e);
            case borealis::SmtEngine::unOp::BNOT:
                return ctx.em.mkExpr(kind::BITVECTOR_NOT, e);
            case borealis::SmtEngine::unOp::UMINUS:
                return ctx.em.mkExpr(kind::BITVECTOR_NEG, e);
            default:
                break;
        }
        UNREACHABLE("Incorrect unOp value");
    }

    inline static expr_t ite(context_t& ctx, expr_t cond, expr_t t, expr_t f) {
        return ctx.em.mkExpr(CVC4::kind::ITE, cond, t, f);
    }

    inline static expr_t extract(context_t& ctx, expr_t lhv, size_t hi, size_t lo){
        return ctx.em.mkExpr(ctx.em.mkConst(CVC4::BitVectorExtract(hi, lo)), lhv);
    }
    inline static expr_t sext(context_t& ctx, expr_t lhv, size_t sz) {
        return ctx.em.mkExpr(ctx.em.mkConst(CVC4::BitVectorSignExtend(sz)), lhv);
    }
    inline static expr_t zext(context_t& ctx, expr_t lhv, size_t sz) {
        return ctx.em.mkExpr(ctx.em.mkConst(CVC4::BitVectorZeroExtend(sz)), lhv);
    }
    inline static expr_t store(context_t& ctx, expr_t arr, expr_t ix, expr_t elem){
        return ctx.em.mkExpr(CVC4::kind::STORE, arr, ix, elem);
    }
    inline static expr_t apply(context_t& ctx, function_t f, const std::vector<expr_t>& args){
        return ctx.em.mkExpr(CVC4::kind::APPLY_UF, f, args);
    }

    inline static expr_t distinct(context_t& ctx, const std::vector<expr_t>& args) {
        return ctx.em.mkExpr(CVC4::kind::DISTINCT, args);
    }

    using Result = smt::Result;

    struct equality {
        bool operator()(expr_t e1, expr_t e2) const noexcept {
            return e1.getId() == e2.getId();
        }
    };
};

}

namespace std {

template<>
struct hash<::CVC4::Expr> {
    size_t operator()(::CVC4::Expr e) const noexcept {
        return static_cast<size_t>(e.getId());
    }
};

} /* namespace std */

#include "Util/unmacros.h"

#endif //CVC4ENGINE_H_H

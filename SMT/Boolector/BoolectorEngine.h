//
// Created by belyaev on 6/7/16.
//

#ifndef BOOLECTOR_ENGINE_H_H
#define BOOLECTOR_ENGINE_H_H

#include <memory>
#include <vector>
#include <cassert>

#include "SMT/Engine.h"

extern "C" {
#include <boolector/boolector.h>

static BoolectorNode *
btor_translate_shift_smt2 (Btor * btor,
                           BoolectorNode * a0,
                           BoolectorNode * a1,
                           BoolectorNode * (* f)(Btor *,
                                                 BoolectorNode *,
                                                 BoolectorNode *))
{
    BoolectorNode * c, * e, * t, * e0, * u, * l, * tmp, * res;
    int len, l0, l1, p0, p1;

    len = boolector_get_width (btor, a0);

    assert (len == boolector_get_width (btor, a1));

    l1 = 0;
    for (l0 = 1; l0 < len; l0 *= 2)
        l1++;

    assert (l0 == (1 << l1));

    if (len == 1)
    {
        assert (l0 == 1);
        assert (l1 == 0);

        if (f != boolector_sra)
        {
            tmp = boolector_not (btor, a1);
            res = boolector_and (btor, a0, tmp);
            boolector_release (btor, tmp);
        }
        else
            res = boolector_copy (btor, a0);
    }
    else
    {
        assert (len >= 1);

        p0 = l0 - len;
        p1 = len - l1;

        assert (p0 >= 0);
        assert (p1 > 0);

        u = boolector_slice (btor, a1, len - 1, len - p1);
        l = boolector_slice (btor, a1, l1 - 1, 0);

        assert (boolector_get_width (btor, u) == p1);
        assert (boolector_get_width (btor, l) == l1);

        if (p1 > 1) c = boolector_redor (btor, u);
        else c = boolector_copy (btor, u);

        boolector_release (btor, u);

        if (f == boolector_sra)
        {
            tmp = boolector_slice (btor, a0, len - 1, len - 1);
            t = boolector_sext (btor, tmp, len - 1);
            boolector_release (btor, tmp);
        }
        else
            t = boolector_zero (btor, len);

        if (!p0) e0 = boolector_copy (btor, a0);
        else if (f == boolector_sra) e0 = boolector_sext (btor, a0, p0);
        else e0 = boolector_uext (btor, a0, p0);

        assert (boolector_get_width (btor, e0) == l0);

        e = f (btor, e0, l);
        boolector_release (btor, e0);
        boolector_release (btor, l);

        if (p0 > 0)
        {
            tmp = boolector_slice (btor, e, len - 1, 0);
            boolector_release (btor, e);
            e = tmp;
        }

        res =  boolector_cond (btor, c, t, e);

        boolector_release (btor, c);
        boolector_release (btor, t);
        boolector_release (btor, e);
    }
    return res;
}


static BoolectorNode* boolector_distinct (Btor* btor, BoolectorNode** nodes, size_t nargs) {
    BoolectorNode* exp = boolector_true (btor);
    for (size_t i = 1; i < nargs; i++)
    {
        for (size_t j = i + 1; j <= nargs; j++)
        {
            BoolectorNode* tmp = boolector_ne (btor, nodes[i], nodes[j]);
            BoolectorNode* old = exp;
            exp = boolector_and (btor, old, tmp);
            boolector_release (btor, old);
            boolector_release (btor, tmp);
        }
    }
    return exp;
}

} /* extern "C" */

#include "SMT/Result.h"

#include "Util/generate_macros.h"
#include "Util/macros.h"

namespace borealis {

namespace boolectorpp {
    struct context{
        Btor* data;

        context(): data(boolector_new()) {}
        context(const context&) = delete;

        ~context() {
            boolector_release_all(data);
            boolector_delete(data);
        }

        operator Btor* () const noexcept {
            return data;
        }
    };

    struct sort_impl {
        id_t class_id;

        sort_impl(id_t derived): class_id(derived){};

        virtual void dump(std::ostream& ost) = 0;
        virtual ~sort_impl(){};
    };
    struct sort{
        std::shared_ptr<sort_impl> impl_;

        sort(std::shared_ptr<sort_impl> impl): impl_(impl) {}
        DEFAULT_CONSTRUCTOR_AND_ASSIGN(sort);

        inline bool is_bool() const;
        inline bool is_bv() const;
        inline bool is_array() const;
        inline bool is_fun() const;
        inline size_t bv_size() const;
        inline sort res() const;
        inline const std::vector<sort>& args() const;
        inline sort ix() const;
        inline sort el() const;

        static sort mk_bool();
        static sort mk_bv(size_t);
        static sort mk_array(sort, sort);
        static sort mk_fun(sort, const std::vector<sort>&);


        friend std::ostream& operator<<(std::ostream& ost, sort s) {
            s.impl_->dump(ost);
            return ost;
        }
    };

    struct bool_sort_impl: sort_impl{
        bool_sort_impl(): sort_impl(class_tag<bool_sort_impl>()){};

        virtual void dump(std::ostream& ost) {
            ost << "bool";
        }
    };
    struct bv_sort_impl: sort_impl{
        bv_sort_impl(size_t bv_size): sort_impl(class_tag<bv_sort_impl>()), bv_size{bv_size}{};
        size_t bv_size;

        virtual void dump(std::ostream& ost) {
            ost << "bv[" << bv_size << "]";
        }
    };
    struct fun_sort_impl: sort_impl{
        fun_sort_impl(sort res, std::vector<sort> args): sort_impl(class_tag<fun_sort_impl>()), res{res}, args{args}{};
        sort res;
        std::vector<sort> args;

        virtual void dump(std::ostream& ost) {
            ost << res << "(" << args << ")";
        }
    };
    struct array_sort_impl: sort_impl {
        array_sort_impl(sort ix, sort el): sort_impl(class_tag<array_sort_impl>()), ix{ix}, el{el}{};
        sort ix;
        sort el;

        virtual void dump(std::ostream& ost) {
            ost << "array<" << ix << ", " << el << ">";
        }
    };

    bool sort::is_bool() const {
        return impl_->class_id == class_tag<bool_sort_impl>();
    }
    bool sort::is_bv() const {
        return impl_->class_id == class_tag<bv_sort_impl>();
    }
    bool sort::is_array() const {
        return impl_->class_id == class_tag<array_sort_impl>();
    }
    bool sort::is_fun() const {
        return impl_->class_id == class_tag<fun_sort_impl>();
    }
    size_t sort::bv_size() const {
        if(is_bool()) return 1;
        return static_cast<bv_sort_impl*>(impl_.get())->bv_size;
    }
    sort sort::res() const {
        return static_cast<fun_sort_impl*>(impl_.get())->res;
    }
    const std::vector<sort>& sort::args() const {
        return static_cast<fun_sort_impl*>(impl_.get())->args;
    }
    sort sort::ix() const {
        return static_cast<array_sort_impl*>(impl_.get())->ix;
    }
    sort sort::el() const {
        return static_cast<array_sort_impl*>(impl_.get())->el;
    }

    struct node {
        context* ctx;
        std::shared_ptr<BoolectorNode> data;
        sort srt;

        node(context* ctx, BoolectorNode* data, sort srt):
            ctx{ctx},
            data{},
            srt{srt}{
            this->data = std::shared_ptr<BoolectorNode>(data, [ctx](auto&& p){ boolector_release(*ctx, p); });
        }
        DEFAULT_CONSTRUCTOR_AND_ASSIGN(node);


        operator BoolectorNode* () const {
            return data.get();
        }
    };

}

struct BoolectorEngine: SmtEngine {
public:
    using expr_t = boolectorpp::node;
    using sort_t = boolectorpp::sort;
    using function_t = expr_t;
    using pattern_t = int;

    struct context_t {
        boolectorpp::context ctx;
        std::unordered_map<std::string, expr_t> symbols;
        size_t key = 0;

        operator Btor*() {
            return ctx;
        }

        operator boolectorpp::context*() {
            return &ctx;
        }

        std::string getFreshName(const std::string& name) {
            auto ret = tfm::format("%s!%d", name, key);
            ++key;
            return std::move(ret);
        }
    };

    using solver_t = boolectorpp::context;

    static std::unique_ptr<context_t> init() {
        return std::unique_ptr<context_t>{ new context_t{} };
    }

    inline static size_t hash(expr_t e) { return boolector_get_id(*e.ctx, e); }
    inline static std::string name(expr_t e) { return boolector_get_symbol(*e.ctx, e); }
    inline static std::string toString(expr_t e) {
        auto&& ctx = *e.ctx;
        if(boolector_is_const(ctx, e)) return boolector_get_bits(*e.ctx, e);

        auto res = boolector_get_symbol(*e.ctx, e);
        if(!res) res = "<boolector node>";
        return res;
    }
    inline static std::string toSMTLib(expr_t e) { return toString(e);  }
    inline static bool term_equality(context_t& ctx, expr_t e1, expr_t e2) {
        return boolector_get_id(ctx, e1) == boolector_get_id(ctx, e2);
    }
    inline static expr_t simplify(context_t&, expr_t e) { return e; }

    inline static sort_t bool_sort(context_t&){ return sort_t::mk_bool(); }
    inline static sort_t bv_sort(context_t&, size_t bitsize){ return sort_t::mk_bv(bitsize); }
    inline static sort_t array_sort(context_t&, sort_t index, sort_t elem){
        return sort_t::mk_array(index, elem);
    }

    inline static sort_t get_sort(context_t&, expr_t e) { return e.srt; }
    inline static size_t bv_size(context_t&, sort_t s) { return s.bv_size(); }
    inline static bool is_bool(context_t&, expr_t e) {
        return e.srt.is_bool();
    }
    inline static bool is_bv(context_t&, expr_t e) {
        return e.srt.is_bv();
    }
    inline static bool is_array(context_t&, expr_t e) {
        return e.srt.is_array();
    }

    inline static pattern_t mkPattern(context_t&, expr_t) {
        UNREACHABLE("patterns not supported by boolector")
    }

    inline static expr_t mkBound(context_t&, size_t, sort_t) {
        UNREACHABLE("bounds not supported by boolector")
    }

    inline static expr_t mkForAll(context_t&, expr_t, expr_t) {
        UNREACHABLE("quantifiers not supported by boolector")
    }

    inline static expr_t mkForAll(
            context_t&,
            const std::vector<sort_t>&,
            std::function<expr_t(const std::vector<expr_t>&)>
    ) {
        UNREACHABLE("quantifiers not supported by boolector")
    }

    inline static expr_t mkForAll(
            context_t&,
            const std::vector<sort_t>&,
            std::function<expr_t(const std::vector<expr_t>&)>,
            std::function<std::vector<pattern_t>(const std::vector<expr_t>&)>
    ) {
        UNREACHABLE("quantifiers not supported by boolector")
    }

    inline static expr_t mkArrayVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        if(fresh == freshness::normal) {
            auto it = ctx.symbols.find(name);
            if(it == std::end(ctx.symbols)) {
                return ctx.symbols[name] = expr_t{
                    ctx,
                    boolector_array(ctx, sort.el().bv_size(), sort.ix().bv_size(), name.c_str()),
                    sort
                };
            } else return it->second;
        } else {
            auto uname = ctx.getFreshName(name);
            return mkArrayVar(ctx, sort, uname, freshness::normal);
        }
    }

    inline static expr_t mkNonArrayVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        if(fresh == freshness::normal) {
            auto it = ctx.symbols.find(name);
            if(it == std::end(ctx.symbols)) {
                return ctx.symbols[name] = expr_t{
                    ctx,
                    boolector_var(ctx, sort.bv_size(), name.c_str()),
                    sort
                };
            } else return it->second;
        } else {
            auto uname = ctx.getFreshName(name);
            return mkNonArrayVar(ctx, sort, uname, freshness::normal);
        }
    }


    inline static expr_t mkVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        if(sort.is_array()) return mkArrayVar(ctx, sort, name, fresh);
        else return mkNonArrayVar(ctx, sort, name, fresh);
    }
    inline static expr_t mkBoolConst(context_t& ctx, bool value) {
        return expr_t{
            ctx,
            value? boolector_true(ctx) : boolector_false(ctx),
            bool_sort(ctx)
        };
    }

    inline static llvm::SmallString<128> asBinString(const llvm::APInt& biggy, size_t size) {
        llvm::SmallString<128> rep;
        biggy.toString(rep, 2, false);
        llvm::SmallString<128> prefix;
        prefix.assign(size - rep.size(), '0');
        prefix.append(rep);
        return prefix;
    }

    template<class Numeric,
             class = std::enable_if_t<std::is_integral<Numeric>::value && std::is_signed<Numeric>::value>
    >
    inline static std::enable_if_t<std::is_integral<Numeric>::value && std::is_signed<Numeric>::value, expr_t> mkNumericConst(context_t& ctx, sort_t sort, Numeric value){
        if(value < 0) {
            auto v = mkNumericConst(ctx, sort, -value);
            return expr_t {
                ctx,
                boolector_neg(ctx, v),
                sort
            };
        } else {
            llvm::APInt biggy(sort.bv_size(), static_cast<uint64_t>(value), false);
            auto rep = asBinString(biggy, sort.bv_size());
            return expr_t {
                ctx,
                boolector_const(ctx, rep.c_str()),
                sort
            };
        }
    }
    template<class Numeric,
             class = std::enable_if_t<std::is_integral<Numeric>::value && std::is_unsigned<Numeric>::value>
    >
    inline static std::enable_if_t<std::is_integral<Numeric>::value && std::is_unsigned<Numeric>::value, expr_t> mkNumericConst(context_t& ctx, sort_t sort, Numeric value){
        llvm::APInt biggy(sort.bv_size(), static_cast<uint64_t>(value), false);
        auto rep = asBinString(biggy, sort.bv_size());
        return expr_t {
            ctx,
            boolector_const(ctx, rep.c_str()),
            sort
        };
    }
    inline static expr_t mkNumericConst(context_t& ctx, sort_t sort, const std::string& valueRep){
        if(sort.is_bool()) return mkBoolConst(ctx, valueRep != "0" && valueRep != "false");
        if(sort.is_bv()) {
            llvm::APInt biggy(sort.bv_size(), valueRep, 10);
            auto rep = asBinString(biggy, sort.bv_size());
            return expr_t {
                ctx,
                boolector_const(ctx, rep.c_str()),
                sort
            };
        }
        UNREACHABLE("illegal sort")
    }

    inline static expr_t mkArrayConst(context_t&, sort_t, expr_t){
        UNREACHABLE("constant arrays are not supported by boolector")
    }
    inline static function_t mkFunction(context_t&, const std::string&, sort_t, const std::vector<sort_t>&) {
        UNREACHABLE("functions are not supported by boolector backend yet")
    }

    inline static function_t mkFreshFunction(context_t&, const std::string&, sort_t, const std::vector<sort_t>&) {
        UNREACHABLE("functions are not supported by boolector backend yet")
    }

    inline static expr_t applyFunction(context_t&, function_t, const std::vector<expr_t>&) {
        UNREACHABLE("functions are not supported by boolector backend yet")
    }

    inline static expr_t conjunction(context_t& ctx) {
        return mkBoolConst(ctx, true);
    };

    template<class HExpr, class ...Exprs>
    inline static expr_t conjunction(context_t& ctx, HExpr h, Exprs... es) {
        return expr_t{
            ctx,
            boolector_and(ctx, h, conjunction(ctx, es...)),
            get_sort(ctx, h)
        };
    }

    template<class ExprContainer>
    inline static expr_t conjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        auto size = ec.size();
        if(size == 0) return mkBoolConst(ctx, true);
        if(size == 1) return util::head(ec);
        if(size == 2) {
            return expr_t{
                ctx,
                boolector_and(ctx, util::head(ec), util::head(util::tail(ec))),
                get_sort(ctx, util::head(ec))
            };
        }

        return expr_t{
            ctx,
            boolector_and(ctx, util::head(ec), conjunctionOfCollection(ctx, util::tail(ec))),
            get_sort(ctx, util::head(ec))
        };
    }

    inline static expr_t disjunction(context_t& ctx) {
        return mkBoolConst(ctx, false);
    };

    template<class HExpr, class ...Exprs>
    inline static expr_t disjunction(context_t& ctx, HExpr h, Exprs... es) {
        return expr_t{
            ctx,
            boolector_or(ctx, h, conjunction(ctx, es...)),
            get_sort(ctx, h)
        };
    }

    template<class ExprContainer>
    inline static expr_t disjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        auto size = ec.size();
        if(size == 0) return mkBoolConst(ctx, false);
        if(size == 1) return util::head(ec);
        if(size == 2) {
            return expr_t{
                ctx,
                boolector_or(ctx, util::head(ec), util::head(util::tail(ec))),
                get_sort(ctx, util::head(ec))
            };
        }

        return expr_t{
            ctx,
            boolector_or(ctx, util::head(ec), conjunctionOfCollection(ctx, util::tail(ec))),
            get_sort(ctx, util::head(ec))
        };
    }

    using Btor_capi_binop = std::add_pointer_t< BoolectorNode*(Btor*, BoolectorNode*, BoolectorNode*) >;
    static Btor_capi_binop binop2btor(binOp bop) {
        static const stdarray_wrapper<Btor_capi_binop, binOp, binOp::LAST> conv = []() {
            stdarray_wrapper<Btor_capi_binop, binOp, binOp::LAST> ret;
            for(auto&& el: ret) el = [](Btor*, BoolectorNode*, BoolectorNode*) -> BoolectorNode* { UNREACHABLE("Incorrect opcode"); };
            ret[binOp::CONJ]    = boolector_and;
            ret[binOp::DISJ]    = boolector_or;
            ret[binOp::IMPLIES] = boolector_implies;
            ret[binOp::IFF]     = boolector_iff;
            ret[binOp::CONCAT]  = boolector_concat;
            ret[binOp::BAND]    = boolector_and;
            ret[binOp::BOR]     = boolector_or;
            ret[binOp::XOR]     = boolector_xor;
            ret[binOp::ADD]     = boolector_add;
            ret[binOp::SUB]     = boolector_sub;
            ret[binOp::SMUL]    = boolector_mul;
            ret[binOp::SDIV]    = boolector_sdiv;
            ret[binOp::SMOD]    = boolector_smod;
            ret[binOp::SREM]    = boolector_srem;
            ret[binOp::UMUL]    = boolector_mul;
            ret[binOp::UDIV]    = boolector_udiv;
            ret[binOp::UMOD]    = boolector_urem;
            ret[binOp::UREM]    = boolector_urem;
            ret[binOp::ASHL]    =
                [](Btor* ctx, BoolectorNode* lhv, BoolectorNode* rhv) -> BoolectorNode* {
                    return btor_translate_shift_smt2(ctx, lhv, rhv, boolector_sll);
                };
            ret[binOp::ASHR]    =
                [](Btor* ctx, BoolectorNode* lhv, BoolectorNode* rhv) -> BoolectorNode* {
                    return btor_translate_shift_smt2(ctx, lhv, rhv, boolector_sra);
                };
            ret[binOp::LSHR]    =
                [](Btor* ctx, BoolectorNode* lhv, BoolectorNode* rhv) -> BoolectorNode* {
                    return btor_translate_shift_smt2(ctx, lhv, rhv, boolector_srl);
                };
            ret[binOp::EQUAL]   = boolector_eq;
            ret[binOp::NEQUAL]  = boolector_ne;
            ret[binOp::SGT]     = boolector_sgt;
            ret[binOp::SLT]     = boolector_slt;
            ret[binOp::SGE]     = boolector_sgte;
            ret[binOp::SLE]     = boolector_slte;
            ret[binOp::UGT]     = boolector_ugt;
            ret[binOp::ULT]     = boolector_ult;
            ret[binOp::UGE]     = boolector_ugte;
            ret[binOp::ULE]     = boolector_ulte;
            ret[binOp::LOAD]    = boolector_read;
            return ret;
        }();
        return conv[bop];
    }

    inline static sort_t calcBinopSort(context_t& ctx, binOp bop, sort_t lhv) {
        switch(bop) {
            case binOp::LOAD:
                return lhv.el();
            case binOp::EQUAL:
            case binOp::NEQUAL:
            case binOp::SGT:
            case binOp::SLT:
            case binOp::SGE:
            case binOp::SLE:
            case binOp::UGT:
            case binOp::ULT:
            case binOp::UGE:
            case binOp::ULE:
                return bool_sort(ctx);
            case binOp::CONJ:
            case binOp::DISJ:
            case binOp::IMPLIES:
            case binOp::IFF:
            case binOp::BAND:
            case binOp::BOR:
            case binOp::XOR:
            case binOp::ADD:
            case binOp::SUB:
            case binOp::SMUL:
            case binOp::SDIV:
            case binOp::SMOD:
            case binOp::SREM:
            case binOp::UMUL:
            case binOp::UDIV:
            case binOp::UMOD:
            case binOp::UREM:
            case binOp::ASHL:
            case binOp::ASHR:
            case binOp::LSHR:
                return lhv;
            default:
                UNREACHABLE("illegal binop")
        }
    }

    inline static expr_t binop(context_t& ctx, binOp bop, expr_t lhv, expr_t rhv) {
        auto f = binop2btor(bop);

        auto res = f(ctx, lhv, rhv);
        size_t bw = boolector_get_width(ctx, res);

        if(bop == binOp::CONCAT) {
            return expr_t(ctx, res, bv_sort(ctx, bw));
        }

        return expr_t(ctx, res, calcBinopSort(ctx, bop, lhv.srt));
    }

    inline static expr_t unop(context_t& ctx, unOp op, expr_t e) {
        std::add_pointer_t< BoolectorNode*(Btor*, BoolectorNode*) > call;

        switch(op) {
            case SmtEngine::unOp::NEGATE:
                call = boolector_not;
                break;
            case SmtEngine::unOp::BNOT:
                call = boolector_not;
                break;
            case SmtEngine::unOp::UMINUS:
                call = boolector_neg;
                break;
            default:
                UNREACHABLE("Incorrect unOp value");
                break;
        }

        return expr_t{ ctx, call(ctx, e), get_sort(ctx, e) };
    }

    inline static expr_t ite(context_t& ctx, expr_t cond, expr_t t, expr_t f) {
        return expr_t{
            ctx,
            boolector_cond(ctx, cond, t, f),
            get_sort(ctx, t)
        };
    }

    inline static expr_t extract(context_t& ctx, expr_t lhv, size_t hi, size_t lo){
        return expr_t{
            ctx,
            boolector_slice(ctx, lhv, hi, lo),
            sort_t::mk_bv(hi - lo + 1)
        };
    }
    inline static expr_t sext(context_t& ctx, expr_t lhv, size_t sz) {
        return expr_t{
            ctx,
            boolector_sext(ctx, lhv, sz),
            sort_t::mk_bv(lhv.srt.bv_size() + sz)
        };
    }
    inline static expr_t zext(context_t& ctx, expr_t lhv, size_t sz) {
        return expr_t{
            ctx,
            boolector_uext(ctx, lhv, sz),
            sort_t::mk_bv(lhv.srt.bv_size() + sz)
        };
    }
    inline static expr_t store(context_t& ctx, expr_t arr, expr_t ix, expr_t elem){
        return expr_t{
            ctx,
            boolector_write(ctx, arr, ix, elem),
            get_sort(ctx, arr)
        };
    }
    inline static expr_t apply(context_t&, function_t, const std::vector<expr_t>&){
        UNREACHABLE("functions are not supported by boolector backend yet")
    }

    inline static expr_t distinct(context_t& ctx, const std::vector<expr_t>& args) {
        std::vector<BoolectorNode*> cargs{ args.begin(), args.end() };
        return expr_t{
            ctx,
            boolector_distinct(ctx, cargs.data(), cargs.size()),
            bool_sort(ctx)
        };
    }

    using Result = smt::Result;

    struct equality {
         bool operator()(expr_t e1, expr_t e2) const noexcept {
             auto&& ctx = e1.ctx;
             return boolector_get_id(*ctx, e1) == boolector_get_id(*ctx, e2);
         }
    };
};

}

namespace std {

template<>
struct hash<borealis::BoolectorEngine::expr_t> {
    size_t operator()(borealis::BoolectorEngine::expr_t e) const noexcept {
        auto&& ctx = e.ctx;
        return static_cast<size_t>(boolector_get_id(*ctx, e));
    }
};

} /* namespace std */

#include "Util/unmacros.h"
#include "Util/generate_unmacros.h"

#endif //BOOLECTOR_ENGINE_H_H

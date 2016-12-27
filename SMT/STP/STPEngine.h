//
// Created by belyaev on 6/7/16.
//

#ifndef STP_ENGINE_H_H
#define STP_ENGINE_H_H

#include <memory>
#include <vector>
#include <cassert>

#include "SMT/Engine.h"

#include <stp/c_interface.h>

#include "SMT/Result.h"

#include "Util/generate_macros.h"
#include "Util/macros.h"

namespace borealis {

namespace stppp {
    struct context{
        VC data;

        context(): data(vc_createValidityChecker()) {}
        context(const context&) = delete;

        ~context() {
            vc_Destroy(data);
        }

        operator VC () const noexcept {
            return data;
        }
    };

    struct sort_impl {
        id_t class_id;

        sort_impl(id_t derived): class_id(derived){};

        virtual void dump(std::ostream& ost) = 0;
        virtual ::Type toSTP(context* ctx) const = 0;
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

        ::Type toSTP(context* ctx) const {
            return impl_->toSTP(ctx);
        }
        friend std::ostream& operator<<(std::ostream& ost, sort s) {
            s.impl_->dump(ost);
            return ost;
        }
    };

    struct bool_sort_impl: sort_impl{
        bool_sort_impl(): sort_impl(class_tag<bool_sort_impl>()){};

        virtual ::Type toSTP(context* ctx) const {
            return vc_boolType(*ctx);
        }

        virtual void dump(std::ostream& ost) {
            ost << "bool";
        }
    };
    struct bv_sort_impl: sort_impl{
        bv_sort_impl(size_t bv_size): sort_impl(class_tag<bv_sort_impl>()), bv_size{bv_size}{};
        size_t bv_size;

        virtual ::Type toSTP(context* ctx) const {
            return vc_bvType(*ctx, bv_size);
        }


        virtual void dump(std::ostream& ost) {
            ost << "bv[" << bv_size << "]";
        }
    };
    struct fun_sort_impl: sort_impl{
        fun_sort_impl(sort res, std::vector<sort> args): sort_impl(class_tag<fun_sort_impl>()), res{res}, args{args}{};
        sort res;
        std::vector<sort> args;

        virtual ::Type toSTP(context*) const {
            UNREACHABLE("UF not supported by stp");
        }

        virtual void dump(std::ostream& ost) {
            ost << res << "(" << args << ")";
        }
    };
    struct array_sort_impl: sort_impl {
        array_sort_impl(sort ix, sort el): sort_impl(class_tag<array_sort_impl>()), ix{ix}, el{el}{};
        sort ix;
        sort el;

        virtual ::Type toSTP(context* ctx) const {
            return vc_arrayType(*ctx, ix.toSTP(ctx), el.toSTP(ctx));
        }

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
        std::shared_ptr<void> data;
        sort srt;

        node(context* ctx, ::Expr data, sort srt):
            ctx{ctx},
            data{},
            srt{srt}{
            this->data = std::shared_ptr<void>(data, [](auto&& p){ vc_DeleteExpr(p); });
        }
        DEFAULT_CONSTRUCTOR_AND_ASSIGN(node);

        operator ::Expr () const {
            return data.get();
        }

        friend std::ostream& operator<<(std::ostream& ost, const node& n) {
            auto rep = exprString(n);
            ON_SCOPE_EXIT(free(rep));
            return ost << rep << "::" << n.srt;
        }

    };

}

struct STPEngine: SmtEngine {
public:
    using expr_t = stppp::node;
    using sort_t = stppp::sort;
    using function_t = expr_t;
    using pattern_t = int;

    struct context_t {
        stppp::context ctx;
        std::unordered_map<std::string, expr_t> symbols;
        std::unordered_map<std::string, std::string> renaming;
        size_t key = 0;

        operator VC() {
            return ctx;
        }

        operator stppp::context*() {
            return &ctx;
        }

        std::string getFreshName(const std::string& name) {
            auto ret = tfm::format("%s!%d", name, key);
            ++key;
            return std::move(ret);
        }

        const std::string& rename(const std::string& name) {
            static int i = 0;
            auto it = renaming.find(name);
            if(it == std::end(renaming)) {
                return renaming[name] = tfm::format("stp%d", ++i);
            } else return it->second;
        }
    };

    using solver_t = stppp::context;

    static std::unique_ptr<context_t> init() {
        return std::unique_ptr<context_t>{ new context_t{} };
    }

    inline static size_t hash(expr_t e) { return static_cast<size_t>(getExprID(e)); }
    inline static std::string name(expr_t e) {
        char* buf;
        ON_SCOPE_EXIT(std::free(buf))
        unsigned long length;
        vc_printExprToBuffer(*e.ctx, e, &buf, &length);
        return std::string(buf, length - 1);
    }
    inline static std::string toString(expr_t e) {
        return name(e);
    }
    inline static std::string toSMTLib(expr_t e) { return toString(e);  }
    inline static bool term_equality(context_t&, expr_t e1, expr_t e2) {
        return getExprID(e1) == getExprID(e2);
    }
    inline static expr_t simplify(context_t& ctx, expr_t e) {
        if(e.srt.is_bool()) { // for some bizarre reason, simplify is not supposed to work on bools in stp
            return e;
        }
        return expr_t{ ctx, vc_simplify(ctx, e), e.srt };
    }

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
        UNREACHABLE("patterns not supported by stp")
    }

    inline static expr_t mkBound(context_t&, size_t, sort_t) {
        UNREACHABLE("bounds not supported by stp")
    }

    inline static expr_t mkForAll(context_t&, expr_t, expr_t) {
        UNREACHABLE("quantifiers not supported by stp")
    }

    inline static expr_t mkForAll(
            context_t&,
            const std::vector<sort_t>&,
            std::function<expr_t(const std::vector<expr_t>&)>
    ) {
        UNREACHABLE("quantifiers not supported by stp")
    }

    inline static expr_t mkForAll(
            context_t&,
            const std::vector<sort_t>&,
            std::function<expr_t(const std::vector<expr_t>&)>,
            std::function<std::vector<pattern_t>(const std::vector<expr_t>&)>
    ) {
        UNREACHABLE("quantifiers not supported by stp")
    }

    inline static expr_t mkArrayVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        if(fresh == freshness::normal) {
            auto it = ctx.symbols.find(name);
            if(it == std::end(ctx.symbols)) {
                return ctx.symbols[name] = expr_t{
                    ctx,
                    vc_varExpr1(ctx, ctx.rename(name).c_str(), sort.ix().bv_size(), sort.el().bv_size()),
                    sort
                };
            } else return it->second;
        } else {
            auto uname = ctx.getFreshName(name);
            return mkArrayVar(ctx, sort, uname, freshness::normal);
        }
    }

    inline static expr_t mkNonArrayVar(context_t& ctx, sort_t sort, const std::string& name, freshness fresh) {
        // std::cerr << name << "::" << sort << std::endl;
        if(fresh == freshness::normal) {
            auto it = ctx.symbols.find(name);
            if(it == std::end(ctx.symbols)) {
                return ctx.symbols[name] = expr_t{
                    ctx,
                    vc_varExpr(ctx, ctx.rename(name).c_str(), sort.toSTP(ctx)),
                    sort
                };
            } else {
                if(sort.toSTP(ctx) != it->second.srt.toSTP(ctx)) {
                    // std::cerr << sort << " -> " << typeString(sort.toSTP(ctx)) << std::endl;
                    // std::cerr << it->second.srt << " -> " << typeString(it->second.srt.toSTP(ctx)) << std::endl;
                }
                return it->second;
            }
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
            value? vc_trueExpr(ctx) : vc_falseExpr(ctx),
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
                vc_bvUMinusExpr(ctx, v),
                sort
            };
        } else {
            llvm::APInt biggy(sort.bv_size(), static_cast<uint64_t>(value), false);
            auto rep = asBinString(biggy, sort.bv_size());
            return expr_t {
                ctx,
                vc_bvConstExprFromStr(ctx, rep.c_str()),
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
            vc_bvConstExprFromStr(ctx, rep.c_str()),
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
                vc_bvConstExprFromStr(ctx, rep.c_str()),
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
        std::vector<::Expr> eps{h, es...};
        return expr_t{
            ctx,
            vc_andExprN(ctx, eps.data(), eps.size()),
            get_sort(ctx, h)
        };
    }

    template<class ExprContainer>
    inline static expr_t conjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        auto size = ec.size();
        if(size == 0) return mkBoolConst(ctx, true);
        if(size == 1) return util::head(ec);

        std::vector<::Expr> eps(ec.begin(), ec.end());
        return expr_t{
            ctx,
            vc_andExprN(ctx, eps.data(), eps.size()),
            get_sort(ctx, util::head(ec))
        };
    }

    inline static expr_t disjunction(context_t& ctx) {
        return mkBoolConst(ctx, false);
    };

    template<class HExpr, class ...Exprs>
    inline static expr_t disjunction(context_t& ctx, HExpr h, Exprs... es) {
        std::vector<::Expr> eps{h, es...};
        return expr_t{
            ctx,
            vc_orExprN(ctx, eps.data(), eps.size()),
            get_sort(ctx, h)
        };
    }

    template<class ExprContainer>
    inline static expr_t disjunctionOfCollection(context_t& ctx, const ExprContainer& ec) {
        auto size = ec.size();
        if(size == 0) return mkBoolConst(ctx, true);
        if(size == 1) return util::head(ec);

        std::vector<::Expr> eps(ec.begin(), ec.end());
        return expr_t{
            ctx,
            vc_orExprN(ctx, eps.data(), eps.size()),
            get_sort(ctx, util::head(ec))
        };
    }

    using STP_standard_api = std::add_pointer_t< ::Expr(::VC, ::Expr, ::Expr) >;
    static STP_standard_api binop2standard(binOp bop) {
        static const stdarray_wrapper<STP_standard_api, binOp, binOp::LAST> conv = []() {
            stdarray_wrapper<STP_standard_api, binOp, binOp::LAST> ret;
            for(auto&& el: ret) el = nullptr;
            ret[binOp::CONJ]    = vc_andExpr;
            ret[binOp::DISJ]    = vc_orExpr;
            ret[binOp::IMPLIES] = vc_impliesExpr;
            ret[binOp::IFF]     = vc_iffExpr;
            ret[binOp::CONCAT]  = vc_bvConcatExpr;
            ret[binOp::BAND]    = vc_bvAndExpr;
            ret[binOp::BOR]     = vc_bvOrExpr;
            ret[binOp::XOR]     = vc_xorExpr;
            ret[binOp::BXOR]    = vc_bvXorExpr;
            ret[binOp::EQUAL]   = vc_eqExpr;
            ret[binOp::NEQUAL]  =
                [](::VC ctx, ::Expr lhv, ::Expr rhv) -> ::Expr {
                    return vc_notExpr(ctx, vc_eqExpr(ctx, lhv, rhv));
                };
            ret[binOp::SGT]     = vc_sbvGtExpr;
            ret[binOp::SLT]     = vc_sbvLtExpr;
            ret[binOp::SGE]     = vc_sbvGeExpr;
            ret[binOp::SLE]     = vc_sbvLeExpr;
            ret[binOp::UGT]     = vc_bvGtExpr;
            ret[binOp::ULT]     = vc_bvLtExpr;
            ret[binOp::UGE]     = vc_bvGeExpr;
            ret[binOp::ULE]     = vc_bvLeExpr;
            ret[binOp::LOAD]    = vc_readExpr;
            return ret;
        }();
        return conv[bop];
    }

    using STP_bv_api = std::add_pointer_t< ::Expr(::VC, int, ::Expr, ::Expr) >;
    static STP_bv_api binop2bv(binOp bop) {
        static const stdarray_wrapper<STP_bv_api, binOp, binOp::LAST> conv = []() {
            stdarray_wrapper<STP_bv_api, binOp, binOp::LAST> ret;
            for(auto&& el: ret) el = nullptr;
            ret[binOp::ADD]     = vc_bvPlusExpr;
            ret[binOp::SUB]     = vc_bvMinusExpr;
            ret[binOp::SMUL]    = vc_bvMultExpr;
            ret[binOp::SDIV]    = vc_sbvDivExpr;
            ret[binOp::SMOD]    = vc_sbvModExpr;
            ret[binOp::SREM]    = vc_sbvRemExpr;
            ret[binOp::UMUL]    = vc_bvMultExpr;
            ret[binOp::UDIV]    = vc_bvDivExpr;
            ret[binOp::UMOD]    = vc_bvModExpr;
            ret[binOp::UREM]    = vc_bvModExpr;
            ret[binOp::ASHL]    = vc_bvLeftShiftExprExpr;
            ret[binOp::ASHR]    = vc_bvSignedRightShiftExprExpr;
            ret[binOp::LSHR]    = vc_bvRightShiftExprExpr;
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
            case binOp::BXOR:
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

        if(bop == binOp::EQUAL && lhv.srt.is_bool()) bop = binOp::IFF;
        if(bop == binOp::NEQUAL && lhv.srt.is_bool()) bop = binOp::XOR;

        if(auto f = binop2standard(bop)) {
            auto res = f(ctx, lhv, rhv);

            if(bop == binOp::CONCAT) {
                size_t bw = vc_getBVLength(ctx, res);
                return expr_t(ctx, res, bv_sort(ctx, bw));
            }

            return expr_t(ctx, res, calcBinopSort(ctx, bop, lhv.srt));
        } else if(auto f = binop2bv(bop)) {
            int size = static_cast<int>(lhv.srt.bv_size());
            auto res = f(ctx, size, lhv, rhv);
            return expr_t(ctx, res, calcBinopSort(ctx, bop, lhv.srt));
        }

        UNREACHABLE("Incorrect opcode")

    }

    inline static expr_t unop(context_t& ctx, unOp op, expr_t e) {
        std::add_pointer_t< ::Expr(::VC, ::Expr) > call;

        switch(op) {
            case SmtEngine::unOp::NEGATE:
                call = vc_notExpr;
                break;
            case SmtEngine::unOp::BNOT:
                call = vc_bvNotExpr;
                break;
            case SmtEngine::unOp::UMINUS:
                call = vc_bvUMinusExpr;
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
            vc_iteExpr(ctx, cond, t, f),
            get_sort(ctx, t)
        };
    }

    inline static expr_t extract(context_t& ctx, expr_t lhv, size_t hi, size_t lo){
        return expr_t{
            ctx,
            vc_bvExtract(ctx, lhv, hi, lo),
            sort_t::mk_bv(hi - lo + 1)
        };
    }
    inline static expr_t sext(context_t& ctx, expr_t lhv, size_t sz) {
        return expr_t{
            ctx,
            vc_bvSignExtend(ctx, lhv, lhv.srt.bv_size() + sz),
            sort_t::mk_bv(lhv.srt.bv_size() + sz)
        };
    }
    inline static expr_t zext(context_t& ctx, expr_t lhv, size_t sz) {
        auto ext = mkNumericConst(ctx, bv_sort(ctx, sz), 0);

        return expr_t{
            ctx,
            vc_bvConcatExpr(ctx, ext, lhv),
            sort_t::mk_bv(lhv.srt.bv_size() + sz)
        };
    }
    inline static expr_t store(context_t& ctx, expr_t arr, expr_t ix, expr_t elem){
        return expr_t{
            ctx,
            vc_writeExpr(ctx, arr, ix, elem),
            get_sort(ctx, arr)
        };
    }
    inline static expr_t apply(context_t&, function_t, const std::vector<expr_t>&){
        UNREACHABLE("functions are not supported by stp backend yet")
    }

    inline static expr_t distinct(context_t& ctx, const std::vector<expr_t>& args) {
        std::vector<expr_t> res;
        for(auto i = 0U; i < args.size(); ++i) {
            for(auto j = i + 1; j < args.size(); ++j) {
                res.push_back(binop(ctx, binOp::NEQUAL, args[i], args[j]));
            }
        }
        return conjunctionOfCollection(ctx, res);
    }

    using Result = smt::Result;

    struct equality {
         bool operator()(expr_t e1, expr_t e2) const noexcept {
             return getExprID(e1) == getExprID(e2);
         }
    };
};

}

namespace std {

template<>
struct hash<borealis::STPEngine::expr_t> {
    size_t operator()(borealis::STPEngine::expr_t e) const noexcept {
        return static_cast<size_t>(getExprID(e));
    }
};

} /* namespace std */

#include "Util/unmacros.h"
#include "Util/generate_unmacros.h"

#endif //STP_ENGINE_H_H

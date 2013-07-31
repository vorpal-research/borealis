/*
 * Logic.hpp
 *
 *  Created on: Jul 31, 2013
 *      Author: Sam Kolton
 */

#ifndef MSAT_LOGIC_HPP_
#define MSAT_LOGIC_HPP_

#include <z3/z3++.h>

#include <functional>

#include "Util/util.h"
#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {
namespace logic {


class Expr {};



namespace msatimpl {
    inline mathsat::Expr defaultAxiom(mathsat::Env& env) {
        return env.bool_val(true);
    }
    inline mathsat::Expr defaultAxiom(mathsat::Expr& e) {
        return defaultAxiom(e.env());
    }
    inline mathsat::Expr spliceAxioms(mathsat::Expr e0, mathsat::Expr e1) {
        return e0 && e1;
    }
    inline mathsat::Expr spliceAxioms(std::initializer_list<mathsat::Expr> il) {
        util::copyref<mathsat::Expr> accum{ util::head(il) };
        for (const auto& e : util::tail(il)) {
            accum = accum && e;
        }
        return accum;
    }
    inline mathsat::Expr spliceAxioms(const std::vector<mathsat::Expr>& v) {
        util::copyref<mathsat::Expr> accum{ util::head(v) };
        for (const auto& e : util::tail(v)) {
            accum = accum && e;
        }
        return accum;
    }
} // namespace msatimpl


////////////////////////////////////////////////////////////////////////////////

class ValueExpr;

namespace msatimpl {
    mathsat::Expr getExpr(const ValueExpr& a);
    mathsat::Expr getAxiom(const ValueExpr& a);
    mathsat::Sort getSort(const ValueExpr& a);
    mathsat::Env& getEnvironment(const ValueExpr& a);
    mathsat::Expr asAxiom(const ValueExpr& e);
    std::string asSmtLib(const ValueExpr& e);

    inline mathsat::Expr getExpr(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getExpr(*a);
    }
    inline mathsat::Expr getAxiom(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getAxiom(*a);
    }
    inline mathsat::Sort getSort(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getSort(*a);
    }
    inline mathsat::Env& getEnvironment(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getEnvironment(*a);
    }
    inline mathsat::Expr asAxiom(const ValueExpr* e) {
        ASSERTC(e != nullptr); return asAxiom(*e);
    }
    inline std::string asSmtLib(const ValueExpr* e) {
        ASSERTC(e != nullptr); return asSmtLib(*e);
    }
    template<class ...Exprs>
    inline mathsat::Expr spliceAxioms(const Exprs&... exprs) {
        return spliceAxioms(
            { getAxiom(exprs)... }
        );
    }
} // namespace msatimpl


class ValueExpr: public Expr {
    struct Impl;
    std::unique_ptr<Impl> pimpl;

public:
    ValueExpr(const ValueExpr&);
    ValueExpr(ValueExpr&&);
    ValueExpr(mathsat::Expr e, mathsat::Expr axiom);
    ValueExpr(mathsat::Expr e);
    ValueExpr(mathsat::Env& env, msat_term ast);
    ~ValueExpr();

    ValueExpr& operator=(const ValueExpr&);

    friend mathsat::Expr msatimpl::getExpr(const ValueExpr& a);
    friend mathsat::Expr msatimpl::getAxiom(const ValueExpr& a);
    friend mathsat::Expr msatimpl::asAxiom(const ValueExpr& a);
    friend mathsat::Sort msatimpl::getSort(const ValueExpr& a);
    friend mathsat::Env& msatimpl::getEnvironment(const ValueExpr& a);

    void swap(ValueExpr&);

    std::string toSmtLib() const;
};

template<class ExprClass>
ExprClass addAxiom(const ExprClass& expr, const ValueExpr& axiom) {
    return ExprClass{
        msatimpl::getExpr(expr),
        msatimpl::spliceAxioms(msatimpl::getAxiom(expr), msatimpl::asAxiom(axiom))
    };
}

std::ostream& operator<<(std::ostream& ost, const ValueExpr& v);

////////////////////////////////////////////////////////////////////////////////

namespace impl {
template<class Aspect>
struct generator;
}

////////////////////////////////////////////////////////////////////////////////

#define ASPECT_BEGIN(CLASS) \
    class CLASS: public ValueExpr { \
        CLASS(mathsat::Env& env, msat_term term): ValueExpr(env, term){ ASSERTC(impl::generator<CLASS>::check(msatimpl::getExpr(this))) }; \
    public: \
        CLASS(mathsat::Expr e): ValueExpr(e){ ASSERTC(impl::generator<CLASS>::check(msatimpl::getExpr(this))) }; \
        CLASS(mathsat::Expr e, mathsat::Expr a): ValueExpr(e, a){ ASSERTC(impl::generator<CLASS>::check(msatimpl::getExpr(this))) }; \
        CLASS(const CLASS&) = default; \
        CLASS(const ValueExpr& e): ValueExpr(e){ ASSERTC(impl::generator<CLASS>::check(msatimpl::getExpr(this))) }; \
        CLASS& operator=(const CLASS&) = default; \
\
        CLASS withAxiom(const ValueExpr& axiom) const { \
            return addAxiom(*this, axiom); \
        } \
\
        static CLASS mkConst(mathsat::Env& env, typename impl::generator<CLASS>::basetype value) { \
            return CLASS{ impl::generator<CLASS>::mkConst(env, value) }; \
        } \
\
        static CLASS mkVar(mathsat::Env& env, const std::string& name) { \
            return CLASS{ env.constant(name.c_str(), impl::generator<CLASS>::sort(env)) }; \
        } \
\
        static CLASS mkVar( \
                mathsat::Env& env, \
                const std::string& name, \
                std::function<Bool(CLASS)> daAxiom) { \
            /* first construct the no-axiom version*/  \
            CLASS nav = mkVar(env, name); \
            return CLASS{ \
                msatimpl::getExpr(nav), \
                msatimpl::spliceAxioms( \
                    msatimpl::getAxiom(nav), \
                    msatimpl::asAxiom(daAxiom(nav)) \
                ) \
            }; \
        } \
\
        static CLASS mkFreshVar(mathsat::Env& env, const std::string& name) { \
            return CLASS{ env.fresh_constant(name.c_str(), impl::generator<CLASS>::sort(env)) }; \
        } \
\
        static CLASS mkFreshVar( \
                mathsat::Env& env, \
                const std::string& name, \
                std::function<Bool(CLASS)> daAxiom) { \
            /* first construct the no-axiom version*/  \
            CLASS nav = mkFreshVar(env, name); \
            return CLASS{ \
                msatimpl::getExpr(nav), \
                msatimpl::spliceAxioms( \
                    msatimpl::getAxiom(nav), \
                    msatimpl::asAxiom(daAxiom(nav)) \
                ) \
            }; \
        }

#define ASPECT_END };

#define ASPECT(CLASS) \
        ASPECT_BEGIN(CLASS) \
        ASPECT_END

////////////////////////////////////////////////////////////////////////////////

class Bool;

namespace impl {
template<>
struct generator<Bool> {
    typedef bool basetype;

    static mathsat::Sort sort(mathsat::Env& env) { return env.bool_sort(); }
    static bool check(mathsat::Expr e) { return e.is_bool(); }
    static mathsat::Expr mkConst(mathsat::Env& env, bool b) { return env.bool_val(b); }
};
} // namespace impl

ASPECT(Bool)

#define REDEF_BOOL_BOOL_OP(OP) \
        Bool operator OP(Bool bv0, Bool bv1);

REDEF_BOOL_BOOL_OP(==)
REDEF_BOOL_BOOL_OP(!=)
REDEF_BOOL_BOOL_OP(&&)
REDEF_BOOL_BOOL_OP(||)
REDEF_BOOL_BOOL_OP(^)

#undef REDEF_BOOL_BOOL_OP

Bool operator!(Bool bv0);

Bool implies(Bool lhv, Bool rhv);
Bool iff(Bool lhv, Bool rhv);

////////////////////////////////////////////////////////////////////////////////

template<size_t N> class BitVector;

namespace impl {
template<size_t N>
struct generator< BitVector<N> > {
    typedef long long basetype;

    static mathsat::Sort sort(mathsat::Env& env) { return env.bv_sort(N)  ; }
    static bool check(mathsat::Expr e) { return e.is_bv() && e.get_sort().bv_size() == N; }
    static mathsat::Expr mkConst(mathsat::Env& env, int n) { return env.bv_val(n, N); }
};
} // namespace impl

template<size_t N>
ASPECT_BEGIN(BitVector)
    enum{ bitsize = N };
    size_t getBitSize() const { return bitsize; }
ASPECT_END

template<size_t N0, size_t N1>
inline
GUARDED(BitVector<N0>, N0 == N1)
grow(BitVector<N1> bv) {
    return bv;
}

template<size_t N0, size_t N1>
inline
GUARDED(BitVector<N0>, N0 > N1)
grow(BitVector<N1> bv) {
    mathsat::Env& env = msatimpl::getEnvironment(bv);

    return BitVector<N0>{
        mathsat::Expr(env, msat_make_bv_sext(env, N0-N1, msatimpl::getExpr(bv))),
        msatimpl::getAxiom(bv)
    };
}

namespace impl {
constexpr size_t max(size_t N0, size_t N1) {
    return N0 > N1 ? N0 : N1;
}
} // namespace impl

template<class T0, class T1>
struct merger;

template<class T>
struct merger<T, T> {
    typedef T type;

    static type app(T t) {
        return t;
    }
};

template<size_t N>
struct merger<BitVector<N>, BitVector<N>> {
    enum{ M = N };
    typedef BitVector<M> type;

    static type app(BitVector<N> bv0) {
        return bv0;
    }
};

template<size_t N0, size_t N1>
struct merger<BitVector<N0>, BitVector<N1>> {
    enum{ M = impl::max(N0,N1) };
    typedef BitVector<M> type;

    static type app(BitVector<N0> bv0) {
        return grow<M>(bv0);
    }

    static type app(BitVector<N1> bv1) {
        return grow<M>(bv1);
    }
};


#define REDEF_BV_BOOL_OP(OP) \
        template<size_t N0, size_t N1, size_t M = impl::max(N0, N1)> \
        Bool operator OP(BitVector<N0> bv0, BitVector<N1> bv1) { \
            auto ebv0 = grow<M>(bv0); \
            auto ebv1 = grow<M>(bv1); \
            return Bool{ \
                msatimpl::getExpr(ebv0) OP msatimpl::getExpr(ebv1), \
                msatimpl::spliceAxioms(bv0, bv1) \
            }; \
        }


REDEF_BV_BOOL_OP(==)
REDEF_BV_BOOL_OP(!=)
REDEF_BV_BOOL_OP(>)
REDEF_BV_BOOL_OP(>=)
REDEF_BV_BOOL_OP(<=)
REDEF_BV_BOOL_OP(<)

#undef REDEF_BV_BOOL_OP


#define REDEF_BV_BIN_OP(OP) \
        template<size_t N0, size_t N1, size_t M = impl::max(N0, N1)> \
        BitVector<M> operator OP(BitVector<N0> bv0, BitVector<N1> bv1) { \
            auto ebv0 = grow<M>(bv0); \
            auto ebv1 = grow<M>(bv1); \
            return BitVector<M>{ \
                msatimpl::getExpr(ebv0) OP msatimpl::getExpr(ebv1), \
                msatimpl::spliceAxioms(bv0, bv1) \
            }; \
        }


REDEF_BV_BIN_OP(+)
REDEF_BV_BIN_OP(-)
REDEF_BV_BIN_OP(*)
REDEF_BV_BIN_OP(/)
REDEF_BV_BIN_OP(|)
REDEF_BV_BIN_OP(&)
REDEF_BV_BIN_OP(^)

#undef REDEF_BV_BIN_OP
//
//
//template<size_t N0, size_t N1, size_t M = impl::max(N0, N1)>
//BitVector<M> operator%(BitVector<N0> bv0, BitVector<N1> bv1) {
//    auto ebv0 = grow<M>(bv0);
//    auto ebv1 = grow<M>(bv1);
//    auto& env = msatimpl::getContext(bv0);
//
//    auto res = z3::to_expr(env, Z3_mk_bvsmod(env, msatimpl::getExpr(ebv0), msatimpl::getExpr(ebv1)));
//    auto axm = msatimpl::spliceAxioms(bv0, bv1);
//    return BitVector<M>{ res, axm };
//}
//
//
//#define REDEF_BV_INT_BIN_OP(OP) \
//        template<size_t N> \
//        BitVector<N> operator OP(BitVector<N> bv, int v1) { \
//            return BitVector<N>{ \
//                msatimpl::getExpr(bv) OP v1, \
//                msatimpl::getAxiom(bv) \
//            }; \
//        }
//
//
//REDEF_BV_INT_BIN_OP(+)
//REDEF_BV_INT_BIN_OP(-)
//REDEF_BV_INT_BIN_OP(*)
//REDEF_BV_INT_BIN_OP(/)
//
//#undef REDEF_BV_INT_BIN_OP
//
//
//#define REDEF_INT_BV_BIN_OP(OP) \
//        template<size_t N> \
//        BitVector<N> operator OP(int v1, BitVector<N> bv) { \
//            return BitVector<N>{ \
//                v1 OP msatimpl::getExpr(bv), \
//                msatimpl::getAxiom(bv) \
//            }; \
//        }
//
//
//REDEF_INT_BV_BIN_OP(+)
//REDEF_INT_BV_BIN_OP(-)
//REDEF_INT_BV_BIN_OP(*)
//REDEF_INT_BV_BIN_OP(/)
//
//#undef REDEF_INT_BV_BIN_OP
//
//
//#define REDEF_UN_OP(OP) \
//        template<size_t N> \
//        BitVector<N> operator OP(BitVector<N> bv) { \
//            return BitVector<N>{ \
//                OP msatimpl::getExpr(bv), \
//                msatimpl::getAxiom(bv) \
//            }; \
//        }
//
//
//REDEF_UN_OP(~)
//REDEF_UN_OP(-)
//
//#undef REDEF_UN_OP
//
//////////////////////////////////////////////////////////////////////////////////
//
//struct ifer {
//    template<class E>
//    struct elser {
//        Bool cond;
//        E tbranch;
//
//        template<class E2>
//        inline typename merger<E,E2>::type else_(E2 fbranch) {
//            auto& env = msatimpl::getContext(cond);
//            auto tb = merger<E,E2>::app(tbranch);
//            auto fb = merger<E,E2>::app(fbranch);
//            return typename merger<E, E2>::type{
//                   z3::to_expr(
//                       env,
//                       Z3_mk_ite(
//                           env,
//                           msatimpl::getExpr(cond),
//                           msatimpl::getExpr(tb),
//                           msatimpl::getExpr(fb)
//                       )
//                   ),
//                   msatimpl::spliceAxioms(cond, tb, fb)
//            };
//        }
//    };
//
//    struct thener {
//        Bool cond;
//
//        template<class E>
//        inline elser<E> then_(E tbranch) {
//            return elser<E>{ cond, tbranch };
//        }
//    };
//
//    thener operator()(Bool cond) {
//        return thener{ cond };
//    }
//};
//
//inline ifer::thener if_(Bool cond) {
//    return ifer()(cond);
//}
//
//template<class T, class U>
//T switch_(
//        U cond,
//        const std::vector<std::pair<U, T>>& cases,
//        T default_
//    ) {
//
//    auto mkIte = [cond](T b, const std::pair<U, T>& a) {
//        return if_(cond == a.first)
//               .then_(a.second)
//               .else_(b);
//    };
//
//    return std::accumulate(cases.begin(), cases.end(), default_, mkIte);
//}
//
//template<class T>
//T switch_(
//        const std::vector<std::pair<Bool, T>>& cases,
//        T default_
//    ) {
//
//    auto mkIte = [](T b, const std::pair<Bool, T>& a) {
//        return if_(a.first)
//               .then_(a.second)
//               .else_(b);
//    };
//
//    return std::accumulate(cases.begin(), cases.end(), default_, mkIte);
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//namespace msatimpl {
//
//template<class Tl, size_t ...N>
//std::tuple< util::index_in_type_list_q<N, Tl>... >
//mkbounds_step_1(mathsat::Env& env, Tl, util::indexer<N...>) {
//    return std::tuple< util::index_in_type_list_q<N, Tl>... > {
//        util::index_in_type_list_q<N, Tl>::mkBound(env, N)...
//    };
//}
//
//template<class ...Args>
//std::tuple<Args...> mkBounds(mathsat::Env& env) {
//    return mkbounds_step_1(env, util::type_list<Args...>(), typename util::make_indexer<Args...>::type());
//}
//
//} // namespace msatimpl
//
//////////////////////////////////////////////////////////////////////////////////
//
//namespace msatimpl {
//
//template<class Res, class ...Args>
//mathsat::Expr forAll(
//        mathsat::Env& env,
//        std::function<Res(Args...)> func
//    ) {
//
//    using borealis::util::toString;
//    using borealis::util::view;
//    std::vector<mathsat::Sort> sorts { impl::generator<Args>::sort(env)... };
//
//    size_t numBounds = sorts.size();
//
//    auto bounds = mkBounds<Args...>(env);
//    auto body = util::apply_packed(func, bounds);
//
//    std::vector<Z3_sort> sort_array(sorts.rbegin(), sorts.rend());
//
//    std::vector<Z3_symbol> name_array;
//    for (size_t i = 0U; i < numBounds; ++i) {
//        std::string name = "forall_bound_" + toString(numBounds - i - 1);
//        name_array.push_back(env.str_symbol(name.c_str()));
//    }
//
//    auto axiom = z3::to_expr(
//            env,
//            Z3_mk_forall(
//                    env,
//                    0,
//                    0,
//                    nullptr,
//                    numBounds,
//                    &sort_array[0],
//                    &name_array[0],
//                    msatimpl::getExpr(body)));
//    return axiom.simplify();
//}
//
//} // namespace msatimpl
//
//////////////////////////////////////////////////////////////////////////////////
//
//class ComparableExpr;
//
//namespace impl {
//template<>
//struct generator<ComparableExpr> : generator<BitVector<1>> {
//    static bool check(mathsat::Expr e) { return e.is_bool() || e.is_bv() || e.is_arith() || e.is_real(); }
//};
//} // namespace impl
//
//ASPECT_BEGIN(ComparableExpr)
//public:
//
//#define DEF_OP(NAME) \
//Bool NAME(const ComparableExpr& other) { \
//    auto& env = msatimpl::getContext(this); \
//    auto l = msatimpl::getExpr(this); \
//    auto r = msatimpl::getExpr(other); \
//    auto res = z3::to_expr(env, Z3_mk_bv##NAME(env, l, r)); \
//    auto axm = msatimpl::spliceAxioms(*this, other); \
//    return Bool{ res, axm }; \
//}
//
//DEF_OP(ugt)
//DEF_OP(uge)
//DEF_OP(ult)
//DEF_OP(ule)
//
//#undef DEF_OP
//
//ASPECT_END
//
//#define REDEF_OP(OP) \
//    Bool operator OP(const ComparableExpr& lhv, const ComparableExpr& rhv);
//
//    REDEF_OP(<)
//    REDEF_OP(>)
//    REDEF_OP(>=)
//    REDEF_OP(<=)
//    REDEF_OP(==)
//    REDEF_OP(!=)
//
//#undef REDEF_OP
//
//////////////////////////////////////////////////////////////////////////////////
//
//class DynBitVectorExpr;
//
//namespace impl {
//template<>
//struct generator<DynBitVectorExpr> : generator<BitVector<1>> {
//    static bool check(mathsat::Expr e) { return e.is_bv(); }
//};
//} // namespace impl
//
//ASPECT_BEGIN(DynBitVectorExpr)
//public:
//    size_t getBitSize() const {
//        return msatimpl::getSort(this).bv_size();
//    }
//
//    DynBitVectorExpr growTo(size_t n) const {
//        size_t m = getBitSize();
//        auto& env = msatimpl::getContext(this);
//        if (m < n)
//            return DynBitVectorExpr{
//                z3::to_expr(env, Z3_mk_sign_ext(env, n-m, msatimpl::getExpr(this))),
//                msatimpl::getAxiom(this)
//            };
//        else return DynBitVectorExpr{ *this };
//    }
//
//    DynBitVectorExpr lshr(const DynBitVectorExpr& shift) {
//        size_t sz = std::max(getBitSize(), shift.getBitSize());
//        DynBitVectorExpr w = this->growTo(sz);
//        DynBitVectorExpr s = shift.growTo(sz);
//        auto& env = msatimpl::getContext(w);
//
//        auto res = z3::to_expr(env, Z3_mk_bvlshr(env, msatimpl::getExpr(w), msatimpl::getExpr(s)));
//        auto axm = msatimpl::spliceAxioms(w, s);
//        return DynBitVectorExpr{ res, axm };
//    }
//ASPECT_END
//
//#define BIN_OP(OP) \
//    DynBitVectorExpr operator OP(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv);
//
//    BIN_OP(+)
//    BIN_OP(-)
//    BIN_OP(*)
//    BIN_OP(/)
//    BIN_OP(|)
//    BIN_OP(&)
//    BIN_OP(^)
//    BIN_OP(%)
//    BIN_OP(>>)
//    BIN_OP(<<)
//
//#undef BIN_OP
//
//////////////////////////////////////////////////////////////////////////////////
//
//// Untyped logic expression
//class SomeExpr: public ValueExpr {
//public:
//    typedef SomeExpr self;
//
//    SomeExpr(mathsat::Expr e): ValueExpr(e) {};
//    SomeExpr(mathsat::Expr e, mathsat::Expr axiom): ValueExpr(e, axiom) {};
//    SomeExpr(const SomeExpr&) = default;
//    SomeExpr(const ValueExpr& b): ValueExpr(b) {};
//
//    static SomeExpr mkDynamic(Bool b) { return SomeExpr{ b }; }
//
//    template<size_t N>
//    static SomeExpr mkDynamic(BitVector<N> bv) { return SomeExpr{ bv }; }
//
//    template<class Aspect>
//    bool is() {
//        return impl::generator<Aspect>::check(msatimpl::getExpr(this));
//    }
//
//    template<class Aspect>
//    util::option<Aspect> to() {
//        using util::just;
//        using util::nothing;
//
//        if (is<Aspect>()) return just( Aspect{ *this } );
//        else return nothing();
//    }
//
//    bool isBool() {
//        return is<Bool>();
//    }
//
//    borealis::util::option<Bool> toBool() {
//        return to<Bool>();
//    }
//
//    template<size_t N>
//    bool isBitVector() {
//        return is<BitVector<N>>();
//    }
//
//    template<size_t N>
//    borealis::util::option<BitVector<N>> toBitVector() {
//        return to<BitVector<N>>();
//    }
//
//    bool isComparable() {
//        return is<ComparableExpr>();
//    }
//
//    borealis::util::option<ComparableExpr> toComparable() {
//        return to<ComparableExpr>();
//    }
//
//    // equality comparison operators are the most general ones
//    Bool operator==(const SomeExpr& that) {
//        return Bool{
//            msatimpl::getExpr(this) == msatimpl::getExpr(that),
//            msatimpl::spliceAxioms(*this, that)
//        };
//    }
//
//    Bool operator!=(const SomeExpr& that) {
//        return Bool{
//            msatimpl::getExpr(this) != msatimpl::getExpr(that),
//            msatimpl::spliceAxioms(*this, that)
//        };
//    }
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
//template<class Elem>
//Bool distinct(mathsat::Env& env, const std::vector<Elem>& elems) {
//    if (elems.empty()) return Bool::mkConst(env, true);
//
//    std::vector<Z3_ast> cast;
//    for (const auto& elem : elems) {
//        cast.push_back(msatimpl::getExpr(elem));
//    }
//
//    mathsat::Expr ret = z3::to_expr(env, Z3_mk_distinct(env, cast.size(), &cast[0]));
//
//    mathsat::Expr axiom = msatimpl::getAxiom(elems[0]);
//    for (const auto& elem : elems) {
//        axiom = msatimpl::spliceAxioms(axiom, msatimpl::getAxiom(elem));
//    }
//
//    return Bool{ ret, axiom };
//}
//
//template<class ...Args>
//Bool forAll(
//        mathsat::Env& env,
//        std::function<Bool(Args...)> func
//    ) {
//    return Bool{ msatimpl::forAll(env, func) };
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//template< class >
//class Function; // undefined
//
//template<class Res, class ...Args>
//class Function<Res(Args...)> : public Expr {
//    z3::func_decl inner;
//    mathsat::Expr axiomatic;
//
//    template<class ...CArgs>
//    inline static mathsat::Expr massAxiomAnd(CArgs... args) {
//        static_assert(sizeof...(args) > 0, "Trying to massAxiomAnd zero axioms");
//        return msatimpl::spliceAxioms({ msatimpl::getAxiom(args)... });
//    }
//
//    static z3::func_decl constructFunc(mathsat::Env& env, const std::string& name) {
//        const size_t N = sizeof...(Args);
//        mathsat::Sort domain[N] = { impl::generator<Args>::sort(env)... };
//
//        return env.function(name.c_str(), N, domain, impl::generator<Res>::sort(env));
//    }
//
//    static z3::func_decl constructFreshFunc(mathsat::Env& env, const std::string& name) {
//        const size_t N = sizeof...(Args);
//        Z3_sort domain[N] = { impl::generator<Args>::sort(env)... };
//        auto fd = Z3_mk_fresh_func_decl(env, name.c_str(), N, domain, impl::generator<Res>::sort(env));
//
//        return z3::func_decl(env, fd);
//    }
//
//    static mathsat::Expr constructAxiomatic(
//            mathsat::Env& env,
//            z3::func_decl z3func,
//            std::function<Res(Args...)> realfunc) {
//
//        std::function<Bool(Args...)> lam = [=](Args... args) -> Bool {
//            return Bool(z3func(msatimpl::getExpr(args)...) == msatimpl::getExpr(realfunc(args...)));
//        };
//
//        return msatimpl::forAll<Bool, Args...>(env, lam);
//    }
//
//public:
//
//    typedef Function Self;
//
//    Function(const Function&) = default;
//    Function(z3::func_decl inner, mathsat::Expr axiomatic):
//        inner(inner), axiomatic(axiomatic) {};
//    explicit Function(z3::func_decl inner):
//        inner(inner), axiomatic(msatimpl::defaultAxiom(inner.env())) {};
//    Function(mathsat::Env& env, const std::string& name, std::function<Res(Args...)> f):
//        inner(constructFunc(env, name)), axiomatic(constructAxiomatic(env, inner, f)){}
//
//    z3::func_decl get() const { return inner; }
//    mathsat::Expr axiom() const { return axiomatic; }
//    mathsat::Env& env() const { return inner.env(); }
//
//    Res operator()(Args... args) const {
//        return Res(inner(msatimpl::getExpr(args)...), msatimpl::spliceAxioms(this->axiom(), massAxiomAnd(args...)));
//    }
//
//    static mathsat::Sort range(mathsat::Env& env) {
//        return impl::generator<Res>::sort(env);
//    }
//
//    template<size_t N = 0>
//    static mathsat::Sort domain(mathsat::Env& env) {
//        return impl::generator< util::index_in_row_q<N, Args...> >::sort(env);
//    }
//
//    static Self mkFunc(mathsat::Env& env, const std::string& name) {
//        return Self{ constructFunc(env, name) };
//    }
//
//    static Self mkFunc(mathsat::Env& env, const std::string& name, std::function<Res(Args...)> body) {
//        z3::func_decl f = constructFunc(env, name);
//        mathsat::Expr ax = constructAxiomatic(env, f, body);
//        return Self{ f, ax };
//    }
//
//    static Self mkFreshFunc(mathsat::Env& env, const std::string& name) {
//        return Self{ constructFreshFunc(env, name) };
//    }
//
//    static Self mkFreshFunc(mathsat::Env& env, const std::string& name, std::function<Res(Args...)> body) {
//        z3::func_decl f = constructFreshFunc(env, name);
//        mathsat::Expr ax = constructAxiomatic(env, f, body);
//        return Self{ f, ax };
//    }
//
//    static Self mkDerivedFunc(
//            mathsat::Env& env,
//            const std::string& name,
//            const Self& oldFunc,
//            std::function<Res(Args...)> body) {
//        z3::func_decl f = constructFreshFunc(env, name);
//        mathsat::Expr ax = constructAxiomatic(env, f, body);
//        return Self{ f, msatimpl::spliceAxioms(oldFunc.axiom(), ax) };
//    }
//
//    static Self mkDerivedFunc(
//            mathsat::Env& env,
//            const std::string& name,
//            const std::vector<Self>& oldFuncs,
//            std::function<Res(Args...)> body) {
//        z3::func_decl f = constructFreshFunc(env, name);
//
//        std::vector<mathsat::Expr> axs;
//        axs.reserve(oldFuncs.size() + 1);
//        std::transform(oldFuncs.begin(), oldFuncs.end(), std::back_inserter(axs),
//            [](const Self& oldFunc) { return oldFunc.axiom(); });
//        axs.push_back(constructAxiomatic(env, f, body));
//
//        return Self{ f, msatimpl::spliceAxioms(axs) };
//    }
//};
//
//template<class Res, class ...Args>
//std::ostream& operator<<(std::ostream& ost, Function<Res(Args...)> f) {
//    return ost << f.get() << " assuming " << f.axiom();
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//template<class Elem, class Index>
//class FuncArray {
//    typedef FuncArray<Elem, Index> Self;
//    typedef Function<Elem(Index)> inner_t;
//
//    std::shared_ptr<std::string> name;
//    inner_t inner;
//
//    FuncArray(const std::string& name, inner_t inner):
//        name(std::make_shared<std::string>(name)), inner(inner) {};
//    FuncArray(std::shared_ptr<std::string> name, inner_t inner):
//        name(name), inner(inner) {};
//
//public:
//
//    FuncArray(const FuncArray&) = default;
//    FuncArray(mathsat::Env& env, const std::string& name):
//        FuncArray(name, inner_t::mkFreshFunc(env, name)) {}
//    FuncArray(mathsat::Env& env, const std::string& name, std::function<Elem(Index)> f):
//        FuncArray(name, inner_t::mkFreshFunc(env, name, f)) {}
//
//    Elem select    (Index i) const { return inner(i);  }
//    Elem operator[](Index i) const { return select(i); }
//
//    Self store(Index i, Elem e) {
//        inner_t nf = inner_t::mkDerivedFunc(env(), *name, inner, [=](Index j) {
//            return if_(j == i).then_(e).else_(this->select(j));
//        });
//
//        return Self{ name, nf };
//    }
//
//    Self store(const std::vector<std::pair<Index, Elem>>& entries) {
//        inner_t nf = inner_t::mkDerivedFunc(env(), *name, inner, [=](Index j) {
//            return switch_(j, entries, this->select(j));
//        });
//
//        return Self{ name, nf };
//    }
//
//    mathsat::Env& env() const { return inner.env(); }
//
//    static Self mkDefault(mathsat::Env& env, const std::string& name, Elem def) {
//        return Self{ env, name, [def](Index){ return def; } };
//    }
//
//    static Self mkFree(mathsat::Env& env, const std::string& name) {
//        return Self{ env, name };
//    }
//
//    static Self merge(
//            const std::string& name,
//            Self defaultArray,
//            const std::vector<std::pair<Bool, Self>>& arrays) {
//
//        std::vector<inner_t> inners;
//        inners.reserve(arrays.size() + 1);
//        std::transform(arrays.begin(), arrays.end(), std::back_inserter(inners),
//            [](const std::pair<Bool, Self>& e) { return e.second.inner; });
//        inners.push_back(defaultArray.inner);
//
//        inner_t nf = inner_t::mkDerivedFunc(defaultArray.env(), name, inners,
//            [=](Index j) -> Elem {
//                std::vector<std::pair<Bool, Elem>> selected;
//                selected.reserve(arrays.size());
//                std::transform(arrays.begin(), arrays.end(), std::back_inserter(selected),
//                    [&j](const std::pair<Bool, Self>& e) {
//                        return std::make_pair(e.first, e.second.select(j));
//                    }
//                );
//
//                return switch_(selected, defaultArray.select(j));
//            }
//        );
//
//        return Self{ name, nf };
//    }
//
//    friend std::ostream& operator<<(std::ostream& ost, const Self& fa) {
//        return ost << "funcArray " << *fa.name << " {" << fa.inner << "}";
//    }
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
//template<class Elem, class Index>
//class TheoryArray: public ValueExpr {
//
//public:
//
//    TheoryArray(mathsat::Expr inner) : ValueExpr(inner) {
//        ASSERT(inner.is_array(), "TheoryArray constructed from non-array");
//    };
//    TheoryArray(mathsat::Expr inner, mathsat::Expr axioms) : ValueExpr(inner, axioms) {
//        ASSERT(inner.is_array(), "TheoryArray constructed from non-array");
//    };
//    TheoryArray(const TheoryArray&) = default;
//
//    Elem select    (Index i) const { return Elem(z3::select(msatimpl::getExpr(this), msatimpl::getExpr(i)));  }
//    Elem operator[](Index i) const { return select(i); }
//
//    TheoryArray store(Index i, Elem e) {
//        return z3::store(msatimpl::getExpr(this), msatimpl::getExpr(i), msatimpl::getExpr(e));
//    }
//
//    TheoryArray store(const std::vector<std::pair<Index, Elem>>& entries) {
//        mathsat::Expr base = msatimpl::getExpr(this);
//        for (const auto& entry : entries) {
//            base = z3::store(base, msatimpl::getExpr(entry.first), msatimpl::getExpr(entry.second));
//        }
//        return base;
//    }
//
//    mathsat::Env& env() const { return msatimpl::getContext(this); }
//
//    static TheoryArray mkDefault(mathsat::Env& env, const std::string&, Elem def) {
//        return z3::const_array(impl::generator<Index>::sort(env), msatimpl::getExpr(def));
//    }
//
//    static TheoryArray mkFree(mathsat::Env& env, const std::string& name) {
//        return env.constant(
//            name.c_str(),
//            env.array_sort(
//                impl::generator<Index>::sort(env),
//                impl::generator<Elem>::sort(env)
//            )
//        );
//    }
//
//    static TheoryArray merge(
//            const std::string&,
//            TheoryArray defaultArray,
//            const std::vector<std::pair<Bool, TheoryArray>>& arrays) {
//        return switch_(arrays, defaultArray);
//    }
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
//template<class Elem, class Index>
//class InlinedFuncArray {
//    typedef InlinedFuncArray<Elem, Index> Self;
//    typedef std::function<Elem(Index)> inner_t;
//
//    mathsat::Env* context;
//    std::shared_ptr<std::string> name;
//    inner_t inner;
//
//    InlinedFuncArray(
//            mathsat::Env& env,
//            std::shared_ptr<std::string> name,
//            inner_t inner
//    ) : context(&env), name(name), inner(inner) {};
//
//public:
//
//    InlinedFuncArray(const InlinedFuncArray&) = default;
//    InlinedFuncArray(mathsat::Env& env, const std::string& name, std::function<Elem(Index)> f):
//        context(&env), name(std::make_shared<std::string>(name)), inner(f) {}
//    InlinedFuncArray(mathsat::Env& env, const std::string& name):
//        context(&env), name(std::make_shared<std::string>(name)) {
//
//        // FIXME belyaev this MAY be generally fucked up, but should work for now
//        // inner = [name,&env](Index ix) -> Elem {
//        //     auto initial = TheoryArray<Elem, Index>::mkFree(env, name + ".initial");
//        //     return initial.select(ix);
//        // };
//
//        // FIXME akhin this is as fucked up as before, but also works for now
//
//        inner = [name,&env](Index ix) -> Elem {
//            auto initial = Function<Elem(Index)>::mkFunc(env, "$$__initial_mem__$$");
//            return initial(ix);
//        };
//    }
//
//    Elem select    (Index i) const { return inner(i);  }
//    Elem operator[](Index i) const { return select(i); }
//
//    InlinedFuncArray& operator=(const InlinedFuncArray&) = default;
//
//    Self store(Index i, Elem e) {
//        inner_t old = this->inner;
//        inner_t nf = [=](Index j) {
//            return if_(j == i).then_(e).else_(old(j));
//        };
//
//        return Self{ *context, name, nf };
//    }
//
//    InlinedFuncArray<Elem, Index> store(const std::vector<std::pair<Index, Elem>>& entries) {
//        inner_t old = this->inner;
//        inner_t nf = [=](Index j) {
//            return switch_(j, entries, old(j));
//        };
//
//        return Self{ *context, name, nf };
//    }
//
//    mathsat::Env& env() const { return *context; }
//
//    static Self mkDefault(mathsat::Env& env, const std::string& name, Elem def) {
//        return Self{ env, name, [def](Index){ return def; } };
//    }
//
//    static Self mkFree(mathsat::Env& env, const std::string& name) {
//        return Self{ env, name };
//    }
//
//    static Self merge(
//            const std::string& name,
//            Self defaultArray,
//            const std::vector<std::pair<Bool, Self>>& arrays) {
//
//        inner_t nf = [=](Index j) -> Elem {
//            std::vector<std::pair<Bool, Elem>> selected;
//            selected.reserve(arrays.size());
//            std::transform(arrays.begin(), arrays.end(), std::back_inserter(selected),
//                [&j](const std::pair<Bool, Self>& e) {
//                    return std::make_pair(e.first, e.second.select(j));
//                }
//            );
//
//            return switch_(selected, defaultArray.select(j));
//        };
//
//        return Self{ defaultArray.env(), name, nf };
//    }
//
//    friend std::ostream& operator<<(std::ostream& ost, const Self& ifa) {
//        return ost << "inlinedArray " << *ifa.name << " {...}";
//    }
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
//template<size_t ElemSize = 8, size_t N>
//std::vector<BitVector<ElemSize>> splitBytes(BitVector<N> bv) {
//    typedef BitVector<ElemSize> Byte;
//
//    if (N <= ElemSize) {
//        return std::vector<Byte>{ grow<ElemSize>(bv) };
//    }
//
//    mathsat::Env& env = msatimpl::getContext(bv);
//
//    std::vector<Byte> ret;
//    for (size_t ix = 0; ix < N; ix += ElemSize) {
//        mathsat::Expr e = z3::to_expr(env, Z3_mk_extract(env, ix+ElemSize-1, ix, msatimpl::getExpr(bv)));
//        ret.push_back(Byte{ e, msatimpl::getAxiom(bv) });
//    }
//    return ret;
//}
//
//template<size_t ElemSize = 8>
//std::vector<BitVector<ElemSize>> splitBytes(SomeExpr bv) {
//    typedef BitVector<ElemSize> Byte;
//
//    auto bv_ = bv.to<DynBitVectorExpr>();
//    ASSERT(!bv_.empty(), "Non-vector");
//
//    DynBitVectorExpr dbv = bv_.getUnsafe();
//    size_t width = dbv.getBitSize();
//
//    if (width <= ElemSize) {
//        SomeExpr newbv = dbv.growTo(ElemSize);
//        auto byte = newbv.to<Byte>();
//        ASSERT(!byte.empty(), "Invalid dynamic BitVector, cannot convert to Byte");
//        return std::vector<Byte>{ byte.getUnsafe() };
//    }
//
//    mathsat::Env& env = msatimpl::getContext(dbv);
//
//    std::vector<Byte> ret;
//    for (size_t ix = 0; ix < width; ix += ElemSize) {
//        mathsat::Expr e = z3::to_expr(env, Z3_mk_extract(env, ix+ElemSize-1, ix, msatimpl::getExpr(dbv)));
//        ret.push_back(Byte{ e, msatimpl::getAxiom(bv) });
//    }
//    return ret;
//}
//
//template<size_t N, size_t ElemSize = 8>
//BitVector<N> concatBytes(const std::vector<BitVector<ElemSize>>& bytes) {
//    typedef BitVector<ElemSize> Byte;
//
//    using borealis::util::toString;
//
//    ASSERT(bytes.size() * ElemSize == N,
//           "concatBytes failed to merge the resulting BitVector: "
//           "expected vector of size " + toString(N/ElemSize) +
//           ", got vector of size " + toString(bytes.size()));
//
//    mathsat::Expr head = msatimpl::getExpr(bytes[0]);
//    mathsat::Env& env = head.env();
//
//    for (size_t i = 1; i < bytes.size(); ++i) {
//        head = mathsat::Expr(env, Z3_mk_concat(env, msatimpl::getExpr(bytes[i]), head));
//    }
//
//    mathsat::Expr axiom = msatimpl::getAxiom(bytes[0]);
//    for (size_t i = 1; i < bytes.size(); ++i) {
//        axiom = msatimpl::spliceAxioms(msatimpl::getAxiom(bytes[i]), axiom);
//    }
//
//    return BitVector<N>{ head, axiom };
//}
//
//template<size_t ElemSize = 8>
//SomeExpr concatBytesDynamic(const std::vector<BitVector<ElemSize>>& bytes) {
//    typedef BitVector<ElemSize> Byte;
//
//    using borealis::util::toString;
//
//    mathsat::Expr head = msatimpl::getExpr(bytes[0]);
//    mathsat::Env& env = head.env();
//
//    for (size_t i = 1; i < bytes.size(); ++i) {
//        head = mathsat::Expr(env, Z3_mk_concat(env, msatimpl::getExpr(bytes[i]), head));
//    }
//
//    mathsat::Expr axiom = msatimpl::getAxiom(bytes[0]);
//    for (size_t i = 1; i < bytes.size(); ++i) {
//        axiom = msatimpl::spliceAxioms(msatimpl::getAxiom(bytes[i]), axiom);
//    }
//
//    return SomeExpr{ head, axiom };
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//template<class Index, size_t ElemSize = 8, template<class, class> class InnerArray = TheoryArray>
//class ScatterArray {
//    typedef BitVector<ElemSize> Byte;
//    typedef InnerArray<Byte, Index> Inner;
//
//    Inner inner;
//
//    ScatterArray(InnerArray<Byte, Index> inner): inner(inner) {};
//
//public:
//
//    ScatterArray(const ScatterArray&) = default;
//    ScatterArray(ScatterArray&&) = default;
//    ScatterArray& operator=(const ScatterArray&) = default;
//    ScatterArray& operator=(ScatterArray&&) = default;
//
//    SomeExpr select(Index i, size_t elemBitSize) {
//        std::vector<Byte> bytes;
//        for (size_t j = 0; j < elemBitSize/ElemSize; ++j) {
//            bytes.push_back(inner[i+j]);
//        }
//        return concatBytesDynamic(bytes);
//    }
//
//    template<class Elem>
//    Elem select(Index i) {
//        enum{ elemBitSize = Elem::bitsize };
//
//        std::vector<Byte> bytes;
//        for (size_t j = 0; j < elemBitSize/ElemSize; ++j) {
//            bytes.push_back(inner[i+j]);
//        }
//        return concatBytes<elemBitSize>(bytes);
//    }
//
//    mathsat::Env& env() const { return inner.env(); }
//
//    Byte operator[](Index i) {
//        return inner[i];
//    }
//
//    Byte operator[](long long i) {
//        return inner[Index::mkConst(env(), i)];
//    }
//
//    template<class Elem>
//    inline ScatterArray store(Index i, Elem e, size_t elemBitSize) {
//        std::vector<Byte> bytes = splitBytes<ElemSize>(e);
//
//        std::vector<std::pair<Index, Byte>> cases;
//        for (size_t j = 0; j < elemBitSize/ElemSize; ++j) {
//            cases.push_back({ i+j, bytes[j] });
//        }
//        return inner.store(cases);
//    }
//
//    template<class Elem>
//    ScatterArray store(Index i, Elem e) {
//        return store(i, e, Elem::bitsize);
//    }
//
//    ScatterArray store(Index i, SomeExpr e) {
//        auto bv = e.to<DynBitVectorExpr>();
//        ASSERTC(!bv.empty());
//        auto bitsize = bv.getUnsafe().getBitSize();
//        return store(i, e, bitsize);
//    }
//
//    static ScatterArray mkDefault(mathsat::Env& env, const std::string& name, Byte def) {
//        return ScatterArray{ Inner::mkDefault(env, name, def) };
//    }
//
//    static ScatterArray mkFree(mathsat::Env& env, const std::string& name) {
//        return ScatterArray{ Inner::mkFree(env, name) };
//    }
//
//    static ScatterArray merge(
//            const std::string& name,
//            ScatterArray defaultArray,
//            const std::vector<std::pair<Bool, ScatterArray>>& arrays
//        ) {
//
//        std::vector<std::pair<Bool, Inner>> inners;
//        inners.reserve(arrays.size());
//        std::transform(arrays.begin(), arrays.end(), std::back_inserter(inners),
//            [](const std::pair<Bool, ScatterArray>& p) {
//                return std::make_pair(p.first, p.second.inner);
//            }
//        );
//
//        Inner merged = Inner::merge(name, defaultArray.inner, inners);
//
//        return ScatterArray{ merged };
//    }
//
//    friend std::ostream& operator<<(std::ostream& ost, const ScatterArray& arr) {
//        return ost << arr.inner;
//    }
//};


} // namespace logic
} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"

#endif  LOGIC_HPP_

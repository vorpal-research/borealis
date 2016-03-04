/*
 * Logic.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: belyaev
 */

#ifndef Z3_LOGIC_HPP
#define Z3_LOGIC_HPP

#include <z3/z3++.h>

#include <functional>

#include "Util/util.h"
#include "Util/functional.hpp"

#include "Config/config.h"

#include "Util/macros.h"

namespace borealis {
namespace z3_ {
namespace logic {

static borealis::config::BoolConfigEntry useProactiveSimplify("z3", "aggressive-simplify");

class Expr {};

struct ConversionException: public std::exception {
    std::string message;

    ConversionException(const std::string& msg);

    virtual const char* what() const throw();
};

namespace z3impl {
    inline z3::expr defaultAxiom(z3::context& ctx) {
        return ctx.bool_val(true);
    }
    inline z3::expr defaultAxiom(z3::expr& e) {
        return defaultAxiom(e.ctx());
    }
    inline z3::expr spliceAxiomsImpl(z3::expr e0, z3::expr e1) {
        if(z3::eq(e0, defaultAxiom(e0))) return e1;
        if(z3::eq(e1, defaultAxiom(e1))) return e0;
        return (e0 && e1);
    }
    inline z3::expr spliceAxiomsImpl(z3::expr e0, z3::expr e1, z3::expr e2) {
        if(z3::eq(e0, defaultAxiom(e0))) return spliceAxiomsImpl(e1, e2);
        if(z3::eq(e1, defaultAxiom(e1))) return spliceAxiomsImpl(e0, e2);
        if(z3::eq(e2, defaultAxiom(e2))) return spliceAxiomsImpl(e0, e1);
        return (e0 && e1 && e2);
    }
    inline z3::expr spliceAxiomsImpl(std::initializer_list<z3::expr> il) {
        auto&& ctx = util::head(il).ctx();
        auto res = util::viewContainer(il).map(ops::static_caster<Z3_ast>()).toVector();

        return z3::expr(ctx, Z3_mk_and(ctx, res.size(), res.data()));
//        util::copyref<z3::expr> accum{ util::head(il) };
//        for (const auto& e : util::tail(il)) {
//            accum = (accum && e);
//        }
//        return accum;
    }
    inline z3::expr spliceAxiomsImpl(const std::vector<z3::expr>& v) {
        auto&& ctx = util::head(v).ctx();
        auto res = util::viewContainer(v).map(ops::static_caster<Z3_ast>()).toVector();
        return z3::expr(ctx, Z3_mk_and(ctx, res.size(), res.data()));
    }
} // namespace z3impl

////////////////////////////////////////////////////////////////////////////////

class ValueExpr;

namespace z3impl {
    z3::expr getExpr(const ValueExpr& a);
    z3::expr getAxiom(const ValueExpr& a);
    z3::sort getSort(const ValueExpr& a);
    z3::context& getContext(const ValueExpr& a);
    std::string getName(const ValueExpr& e);
    size_t getHash(const ValueExpr& e);
    z3::expr asAxiom(const ValueExpr& e);
    std::string asSmtLib(const ValueExpr& e);

    inline z3::expr getExpr(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getExpr(*a);
    }
    inline z3::expr getAxiom(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getAxiom(*a);
    }
    inline z3::sort getSort(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getSort(*a);
    }
    inline z3::context& getContext(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getContext(*a);
    }
    inline std::string getName(const ValueExpr* e) {
        ASSERTC(e != nullptr); return getName(*e);
    }
    inline size_t getHash(const ValueExpr* e) {
        ASSERTC(e != nullptr); return getHash(*e);
    }
    inline z3::expr asAxiom(const ValueExpr* e) {
        ASSERTC(e != nullptr); return asAxiom(*e);
    }
    inline std::string asSmtLib(const ValueExpr* e) {
        ASSERTC(e != nullptr); return asSmtLib(*e);
    }
} // namespace z3impl

class ValueExpr: public Expr {
    struct Impl {
        z3::expr inner;
        z3::expr axiomatic;
    } impl;
    //std::unique_ptr<Impl> pimpl;

public:
    ValueExpr(const ValueExpr&);
    ValueExpr(ValueExpr&&);
    ValueExpr(z3::expr e, z3::expr axiom);
    ValueExpr(z3::expr e);
    ValueExpr(z3::context& ctx, Z3_ast ast);
    ~ValueExpr();

    ValueExpr& operator=(const ValueExpr&);

    friend z3::expr z3impl::getExpr(const ValueExpr& a);
    friend z3::expr z3impl::getAxiom(const ValueExpr& a);
    friend z3::expr z3impl::asAxiom(const ValueExpr& a);
    friend z3::sort z3impl::getSort(const ValueExpr& a);
    friend z3::context& z3impl::getContext(const ValueExpr& a);
    friend std::string z3impl::getName(const ValueExpr& a);
    friend size_t z3impl::getHash(const ValueExpr& a);

    void swap(ValueExpr&);

    std::string getName() const;
    std::string toSmtLib() const;

    size_t getHash() const;

    static bool eq(const ValueExpr& a, const ValueExpr& b);

    ValueExpr simplify() const;
};

namespace z3impl {

// FIXME: this overload is genuinly stupid, but optimizes smth
template<class Expr0, class Expr1>
inline z3::expr spliceAxioms(const Expr0& e0, const Expr1& e1) {
    return spliceAxiomsImpl(getAxiom(e0), getAxiom(e1));
};

template<class Expr0, class Expr1, class Expr2>
inline z3::expr spliceAxioms(const Expr0& e0, const Expr1& e1, const Expr2& e2) {
    return spliceAxiomsImpl(getAxiom(e0), getAxiom(e1), getAxiom(e2));
};

template<class ...Exprs>
inline z3::expr spliceAxioms(const Exprs&... exprs) {
    return spliceAxiomsImpl(
        { getAxiom(exprs)... }
    );
}

} // namespace z3impl

template<class ExprClass>
ExprClass addAxiom(const ExprClass& expr, const ValueExpr& axiom) {
    return ExprClass{
        z3impl::getExpr(expr),
        z3impl::spliceAxiomsImpl(z3impl::getAxiom(expr), z3impl::asAxiom(axiom))
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
        CLASS(z3::context& ctx, Z3_ast ast): ValueExpr(ctx, ast){ ASSERTC(impl::generator<CLASS>::check(z3impl::getExpr(this))) }; \
    public: \
        CLASS(z3::expr e): ValueExpr(e){ ASSERTC(impl::generator<CLASS>::check(z3impl::getExpr(this))) }; \
        CLASS(z3::expr e, z3::expr a): ValueExpr(e, a){ ASSERTC(impl::generator<CLASS>::check(z3impl::getExpr(this))) }; \
        CLASS(const CLASS&) = default; \
        CLASS(const ValueExpr& e): ValueExpr(e){ ASSERTC(impl::generator<CLASS>::check(z3impl::getExpr(this))) }; \
        CLASS& operator=(const CLASS&) = default; \
        CLASS simplify() const { return CLASS{ ValueExpr::simplify() }; } \
\
        CLASS withAxiom(const ValueExpr& axiom) const { \
            return addAxiom(*this, axiom); \
        } \
\
        static CLASS mkConst(z3::context& ctx, typename impl::generator<CLASS>::basetype value) { \
            return CLASS{ impl::generator<CLASS>::mkConst(ctx, value) }; \
        } \
\
        static CLASS mkBound(z3::context& ctx, unsigned i) { \
            return CLASS{ ctx, Z3_mk_bound(ctx, i, impl::generator<CLASS>::sort(ctx)) }; \
        } \
\
        static CLASS mkVar(z3::context& ctx, const std::string& name) { \
            return CLASS{ ctx.constant(name.c_str(), impl::generator<CLASS>::sort(ctx)) }; \
        } \
\
        static CLASS mkVar( \
                z3::context& ctx, \
                const std::string& name, \
                std::function<Bool(CLASS)> daAxiom) { \
            /* first construct the no-axiom version */ \
            CLASS nav = mkVar(ctx, name); \
            return CLASS{ \
                z3impl::getExpr(nav), \
                z3impl::spliceAxiomsImpl( \
                    z3impl::getAxiom(nav), \
                    z3impl::asAxiom(daAxiom(nav)) \
                ) \
            }; \
        } \
\
        static CLASS mkFreshVar(z3::context& ctx, const std::string& name) { \
            return CLASS{ ctx, Z3_mk_fresh_const(ctx, name.c_str(), impl::generator<CLASS>::sort(ctx)) }; \
        } \
\
        static CLASS mkFreshVar( \
                z3::context& ctx, \
                const std::string& name, \
                std::function<Bool(CLASS)> daAxiom) { \
            /* first construct the no-axiom version */ \
            CLASS nav = mkFreshVar(ctx, name); \
            return CLASS{ \
                z3impl::getExpr(nav), \
                z3impl::spliceAxiomsImpl( \
                    z3impl::getAxiom(nav), \
                    z3impl::asAxiom(daAxiom(nav)) \
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

    static z3::sort sort(z3::context& ctx) { return ctx.bool_sort(); }
    static bool check(z3::expr e) { return e.is_bool(); }
    static z3::expr mkConst(z3::context& ctx, bool b) { return ctx.bool_val(b); }
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

    static z3::sort sort(z3::context& ctx) { return ctx.bv_sort(N); }
    static bool check(z3::expr e) { return e.is_bv() && e.get_sort().bv_size() == N; }
    static z3::expr mkConst(z3::context& ctx, int n) { return ctx.bv_val(n, N); }
};
} // namespace impl

template<size_t N>
ASPECT_BEGIN(BitVector)
    enum{ bitsize = N };
    size_t getBitSize() const { return bitsize; }
ASPECT_END

// FIXME: Implement zgrow (zext)

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
    z3::context& ctx = z3impl::getContext(bv);

    return BitVector<N0>{
        z3::to_expr(ctx, Z3_mk_sign_ext(ctx, N0-N1, z3impl::getExpr(bv))),
        z3impl::getAxiom(bv)
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
                z3impl::getExpr(ebv0) OP z3impl::getExpr(ebv1), \
                z3impl::spliceAxioms(bv0, bv1) \
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
                z3impl::getExpr(ebv0) OP z3impl::getExpr(ebv1), \
                z3impl::spliceAxioms(bv0, bv1) \
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


template<size_t N0, size_t N1, size_t M = impl::max(N0, N1)>
BitVector<M> operator%(BitVector<N0> bv0, BitVector<N1> bv1) {
    auto ebv0 = grow<M>(bv0);
    auto ebv1 = grow<M>(bv1);
    auto& ctx = z3impl::getContext(bv0);

    auto res = z3::to_expr(ctx, Z3_mk_bvsmod(ctx, z3impl::getExpr(ebv0), z3impl::getExpr(ebv1)));
    auto axm = z3impl::spliceAxioms(bv0, bv1);
    return BitVector<M>{ res, axm };
}


#define REDEF_BV_INT_BIN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(BitVector<N> bv, int v1) { \
            return BitVector<N>{ \
                z3impl::getExpr(bv) OP v1, \
                z3impl::getAxiom(bv) \
            }; \
        }


REDEF_BV_INT_BIN_OP(+)
REDEF_BV_INT_BIN_OP(-)
REDEF_BV_INT_BIN_OP(*)
REDEF_BV_INT_BIN_OP(/)

#undef REDEF_BV_INT_BIN_OP


#define REDEF_INT_BV_BIN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(int v1, BitVector<N> bv) { \
            return BitVector<N>{ \
                v1 OP z3impl::getExpr(bv), \
                z3impl::getAxiom(bv) \
            }; \
        }


REDEF_INT_BV_BIN_OP(+)
REDEF_INT_BV_BIN_OP(-)
REDEF_INT_BV_BIN_OP(*)
REDEF_INT_BV_BIN_OP(/)

#undef REDEF_INT_BV_BIN_OP


#define REDEF_UN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(BitVector<N> bv) { \
            return BitVector<N>{ \
                OP z3impl::getExpr(bv), \
                z3impl::getAxiom(bv) \
            }; \
        }


REDEF_UN_OP(~)
REDEF_UN_OP(-)

#undef REDEF_UN_OP

////////////////////////////////////////////////////////////////////////////////

struct ifer {
    template<class E>
    struct elser {
        Bool cond;
        E tbranch;

        template<class E2>
        inline typename merger<E,E2>::type else_(E2 fbranch) {
            auto& ctx = z3impl::getContext(cond);
            auto tb = merger<E,E2>::app(tbranch);
            auto fb = merger<E,E2>::app(fbranch);
            return typename merger<E, E2>::type{
                   z3::to_expr(
                       ctx,
                       Z3_mk_ite(
                           ctx,
                           z3impl::getExpr(cond),
                           z3impl::getExpr(tb),
                           z3impl::getExpr(fb)
                       )
                   ),
                   z3impl::spliceAxioms(cond, tb, fb)
            };
        }
    };

    struct thener {
        Bool cond;

        template<class E>
        inline elser<E> then_(E tbranch) {
            return elser<E>{ cond, tbranch };
        }
    };

    thener operator()(Bool cond) {
        return thener{ cond };
    }
};

inline ifer::thener if_(Bool cond) {
    return ifer()(cond);
}

template<class T, class U>
T switch_(
        U cond,
        const std::vector<std::pair<U, T>>& cases,
        T default_
    ) {

    auto mkIte = [cond](T b, const std::pair<U, T>& a) {
        return if_(cond == a.first)
               .then_(a.second)
               .else_(b);
    };

    return std::accumulate(cases.begin(), cases.end(), default_, mkIte);

}

template<class T>
T switch_(
        const std::vector<std::pair<Bool, T>>& cases,
        T default_
    ) {

    auto mkIte = [](T b, const std::pair<Bool, T>& a) {
        return if_(a.first)
               .then_(a.second)
               .else_(b);
    };

    return std::accumulate(cases.begin(), cases.end(), default_, mkIte);
}

////////////////////////////////////////////////////////////////////////////////

namespace z3impl {

template<class Tl, size_t ...N>
std::tuple< util::index_in_type_list_q<N, Tl>... >
mkbounds_step_1(z3::context& ctx, Tl, util::indexer<N...>) {
    return std::tuple< util::index_in_type_list_q<N, Tl>... > {
        util::index_in_type_list_q<N, Tl>::mkBound(ctx, N)...
    };
}

template<class ...Args>
std::tuple<Args...> mkBounds(z3::context& ctx) {
    return mkbounds_step_1(ctx, util::type_list<Args...>(), typename util::make_indexer<Args...>::type());
}

} // namespace z3impl

////////////////////////////////////////////////////////////////////////////////

namespace z3impl {

template<class Expr>
Z3_pattern make_pattern(z3::context& ctx, Expr e) {
    Z3_ast pats[1];
    pats[0] = getExpr(e);
    return Z3_mk_pattern(ctx, 1, pats);
}

template<class Res, class ...Args>
z3::expr forAll(
        z3::context& ctx,
        std::function<Res(Args...)> func
) {

    using borealis::util::toString;
    using borealis::util::view;
    std::vector<z3::sort> sorts { impl::generator<Args>::sort(ctx)... };

    size_t numBounds = sorts.size();

    auto bounds = mkBounds<Args...>(ctx);
    auto body = as_packed(func)(bounds);

    std::vector<Z3_sort> sort_array(sorts.rbegin(), sorts.rend());

    std::vector<Z3_symbol> name_array;
    for (size_t i = 0U; i < numBounds; ++i) {
        std::string name = "forall_bound_" + toString(numBounds - i - 1);
        name_array.push_back(ctx.str_symbol(name.c_str()));
    }

    auto axiom = z3::to_expr(
            ctx,
            Z3_mk_forall(
                    ctx,
                    0,
                    0,
                    0,
                    numBounds,
                    &sort_array[0],
                    &name_array[0],
                    z3impl::getExpr(body)));
    return axiom;
}

template<class VarT>
z3::expr forAllConst(z3::context& /*ctx*/, VarT var, Bool body) {
    return z3::forall(getExpr(var), getExpr(body));
}

template<class Res, class Patterns, class ...Args>
z3::expr forAll(
        z3::context& ctx,
        std::function<Res(Args...)> func,
        std::function<Patterns(Args...)> patternGenerator
    ) {

    using borealis::util::toString;
    using borealis::util::view;
    std::vector<z3::sort> sorts { impl::generator<Args>::sort(ctx)... };

    size_t numBounds = sorts.size();

    auto bounds = mkBounds<Args...>(ctx);
    auto body = as_packed(func)(bounds);

    std::vector<Z3_sort> sort_array(sorts.rbegin(), sorts.rend());

    std::vector<Z3_symbol> name_array;
    for (size_t i = 0U; i < numBounds; ++i) {
        std::string name = "forall_bound_" + toString(numBounds - i - 1);
        name_array.push_back(ctx.str_symbol(name.c_str()));
    }

    auto patterns = as_packed(patternGenerator)(bounds);
    std::vector<Z3_pattern> pattern_array = util::viewContainer(patterns).map(LAM(Expr, make_pattern(ctx, Expr))).toVector();

    auto axiom = z3::to_expr(
            ctx,
            Z3_mk_forall(
                    ctx,
                    0,
                    pattern_array.size(),
                    pattern_array.data(),
                    numBounds,
                    &sort_array[0],
                    &name_array[0],
                    z3impl::getExpr(body)));
    return axiom;
}

} // namespace z3impl

////////////////////////////////////////////////////////////////////////////////

class ComparableExpr;

namespace impl {
template<>
struct generator<ComparableExpr> : generator<BitVector<1>> {
    static bool check(z3::expr e) { return e.is_bool() || e.is_bv() || e.is_arith() || e.is_real(); }
};
} // namespace impl

ASPECT(ComparableExpr)

#define REDEF_OP(OP) \
    Bool operator OP(const ComparableExpr& lhv, const ComparableExpr& rhv);

    REDEF_OP(<)
    REDEF_OP(>)
    REDEF_OP(>=)
    REDEF_OP(<=)
    REDEF_OP(==)
    REDEF_OP(!=)

#undef REDEF_OP

////////////////////////////////////////////////////////////////////////////////

class DynBitVectorExpr;

namespace impl {
template<>
struct generator<DynBitVectorExpr> : generator<BitVector<1>> {
    static bool check(z3::expr e) { return e.is_bv(); }
};
} // namespace impl

ASPECT_BEGIN(DynBitVectorExpr)
public:
    size_t getBitSize() const {
        return z3impl::getSort(this).bv_size();
    }

    DynBitVectorExpr growTo(size_t n) const {
        size_t m = getBitSize();
        auto& ctx = z3impl::getContext(this);
        if (m < n)
            return DynBitVectorExpr{
                z3::to_expr(ctx, Z3_mk_sign_ext(ctx, n-m, z3impl::getExpr(this))),
                z3impl::getAxiom(this)
            };
        else return DynBitVectorExpr{ *this };
    }

    DynBitVectorExpr zgrowTo(size_t n) const {
        size_t m = getBitSize();
        auto& ctx = z3impl::getContext(this);
        if (m < n)
            return DynBitVectorExpr{
                z3::to_expr(ctx, Z3_mk_zero_ext(ctx, n-m, z3impl::getExpr(this))),
                z3impl::getAxiom(this)
            };
        else return DynBitVectorExpr{ *this };
    }

    DynBitVectorExpr extract(size_t high, size_t low) const {
        size_t m = getBitSize();
        ASSERT(high < m, "High must be less then bit-vector size.");
        ASSERT(low <= high, "Low mustn't be greater then high.");

        auto& ctx = z3impl::getContext(this);
        return DynBitVectorExpr{
            z3::to_expr(ctx, Z3_mk_extract(ctx, high, low, z3impl::getExpr(this))),
            z3impl::getAxiom(this)
        };
    }

    DynBitVectorExpr adapt(size_t N, bool isSigned = false) const {
        if (N > getBitSize()) {
            return isSigned ? growTo(N) : zgrowTo(N);
        } else if (N < getBitSize()) {
            return extract(N - 1, 0);
        } else {
            return *this;
        }
    }

    template<class Expr>
    Expr adapt(GUARDED(void*, Expr::bitsize >= 0) = nullptr, bool isSigned = false) const {
        return adapt(Expr::bitsize, isSigned);
    }

    DynBitVectorExpr lshr(const DynBitVectorExpr& shift) {
        size_t sz = std::max(getBitSize(), shift.getBitSize());
        DynBitVectorExpr w = this->growTo(sz);
        DynBitVectorExpr s = shift.growTo(sz);
        auto& ctx = z3impl::getContext(w);

        auto res = z3::to_expr(ctx, Z3_mk_bvlshr(ctx, z3impl::getExpr(w), z3impl::getExpr(s)));
        auto axm = z3impl::spliceAxioms(w, s);
        return DynBitVectorExpr{ res, axm };
    }

    static DynBitVectorExpr mkSizedConst(z3::context& ctx, long long value, size_t bitSize) {
        return DynBitVectorExpr{ ctx.bv_val(value, bitSize) };
    }

    static DynBitVectorExpr mkSizedVar(z3::context& ctx, const std::string& name, size_t bitSize) {
        return DynBitVectorExpr{ ctx.bv_const(name.c_str(), bitSize) };
    }

    static DynBitVectorExpr mkFreshSizedVar(z3::context& ctx, const std::string& name, size_t bitSize) {
        return DynBitVectorExpr{ ctx, Z3_mk_fresh_const(ctx, name.c_str(), ctx.bv_sort(bitSize)) };
    }

ASPECT_END

#define BIN_OP(OP) \
    DynBitVectorExpr operator OP(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv);

    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)
    BIN_OP(|)
    BIN_OP(&)
    BIN_OP(^)
    BIN_OP(%)
    BIN_OP(>>)
    BIN_OP(<<)

#undef BIN_OP

#define BIN_OP(OP) \
    template<size_t N> \
    BitVector<N> operator OP(const DynBitVectorExpr& lhv, const BitVector<N>& rhv) { \
        return BitVector<N>(lhv.adapt(N)) OP rhv; \
    }

    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)

#undef BIN_OP

#define BIN_OP(OP) \
    template<size_t N> \
    BitVector<N> operator OP(const BitVector<N>& lhv, const DynBitVectorExpr& rhv) { \
        return lhv OP BitVector<N>(rhv.adapt(N)); \
    }

    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)

#undef BIN_OP

#define BIN_OP(OP) \
    DynBitVectorExpr operator OP(const DynBitVectorExpr& lhv, int rhv);

    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)

#undef BIN_OP

#define BIN_OP(OP) \
    DynBitVectorExpr operator OP(int lhv, const DynBitVectorExpr& rhv);

    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)

#undef BIN_OP

#define BOOL_OP(OP) \
    Bool operator OP(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv);

    BOOL_OP(==)
    BOOL_OP(!=)
    BOOL_OP(>)
    BOOL_OP(>=)
    BOOL_OP(<)
    BOOL_OP(<=)

#undef BOOL_OP

#define UN_OP(OP) \
    DynBitVectorExpr operator OP(const DynBitVectorExpr& lhv);

    UN_OP(-)
    UN_OP(~)

#undef UN_OP



template<size_t N>
struct merger<BitVector<N>, DynBitVectorExpr> {
    typedef BitVector<N> type;

    static type app(BitVector<N> bv0) {
        return bv0;
    }

    static type app(DynBitVectorExpr bv1) {
        return bv1.adapt(N);
    }
};

template<size_t N>
struct merger<DynBitVectorExpr, BitVector<N>> {
    typedef BitVector<N> type;

    static type app(DynBitVectorExpr bv0) {
        return bv0.adapt(N);
    }

    static type app(BitVector<N> bv1) {
        return bv1;
    }
};

////////////////////////////////////////////////////////////////////////////////

class UComparableExpr;

namespace impl {
template<>
struct generator<UComparableExpr> : generator<BitVector<1>> {
    static bool check(z3::expr e) { return e.is_bv(); }
};
} // namespace impl

ASPECT_BEGIN(UComparableExpr)
public:

#define DEF_OP(NAME) \
    Bool NAME(const UComparableExpr& other) { \
        auto&& ctx = z3impl::getContext(this); \
        auto&& ll = DynBitVectorExpr(*this); \
        auto&& rr = DynBitVectorExpr(other); \
        auto&& sz = std::max(ll.getBitSize(), rr.getBitSize()); \
        auto&& l = z3impl::getExpr(ll.growTo(sz)); \
        auto&& r = z3impl::getExpr(rr.growTo(sz)); \
        auto&& res = z3::to_expr(ctx, Z3_mk_bv##NAME(ctx, l, r)); \
        auto&& axm = z3impl::spliceAxioms(*this, other); \
        return Bool{ res, axm }; \
    }

    DEF_OP(ugt)
    DEF_OP(uge)
    DEF_OP(ult)
    DEF_OP(ule)

#undef DEF_OP

ASPECT_END

////////////////////////////////////////////////////////////////////////////////

// Untyped logic expression
class SomeExpr: public ValueExpr {
public:
    typedef SomeExpr self;

    SomeExpr(z3::expr e): ValueExpr(e) {};
    SomeExpr(z3::expr e, z3::expr axiom): ValueExpr(e, axiom) {};
    SomeExpr(const SomeExpr&) = default;
    SomeExpr(const ValueExpr& b): ValueExpr(b) {};

    SomeExpr withAxiom(const ValueExpr& axiom) const {
        return addAxiom(*this, axiom);
    }

    static SomeExpr mkDynamic(Bool b) { return SomeExpr{ b }; }

    template<size_t N>
    static SomeExpr mkDynamic(BitVector<N> bv) { return SomeExpr{ bv }; }

    template<class Aspect>
    bool is() {
        return impl::generator<Aspect>::check(z3impl::getExpr(this));
    }

    template<class Aspect>
    util::option<Aspect> to() {
        using util::just;
        using util::nothing;

        if (is<Aspect>()) return just( Aspect{ *this } );
        else return nothing();
    }

    bool isBool() {
        return is<Bool>();
    }

    borealis::util::option<Bool> toBool() {
        return to<Bool>();
    }

    template<size_t N>
    bool isBitVector() {
        return is<BitVector<N>>();
    }

    template<size_t N>
    borealis::util::option<BitVector<N>> toBitVector() {
        return to<BitVector<N>>();
    }

    bool isComparable() {
        return is<ComparableExpr>();
    }

    borealis::util::option<ComparableExpr> toComparable() {
        return to<ComparableExpr>();
    }

    bool isUnsignedComparable() {
        return is<UComparableExpr>();
    }

    borealis::util::option<UComparableExpr> toUnsignedComparable() {
        return to<UComparableExpr>();
    }

    // equality comparison operators are the most general ones
    Bool operator==(const SomeExpr& that) {
        return Bool{
            z3impl::getExpr(this) == z3impl::getExpr(that),
            z3impl::spliceAxioms(*this, that)
        };
    }

    Bool operator!=(const SomeExpr& that) {
        return Bool{
            z3impl::getExpr(this) != z3impl::getExpr(that),
            z3impl::spliceAxioms(*this, that)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct add_no_overflow {
    static DynBitVectorExpr doit(DynBitVectorExpr bv0, DynBitVectorExpr bv1, bool isSigned) {
        auto& ctx = z3impl::getContext(bv0);

        auto sz = std::max(bv0.getBitSize(), bv1.getBitSize());
        auto ebv0 = bv0.adapt(sz, isSigned);
        auto ebv1 = bv1.adapt(sz, isSigned);

        auto zero = DynBitVectorExpr{ ctx.bv_val(0, sz) };
        auto zero_ = DynBitVectorExpr{ ctx.bv_val(0, 1) };

        if (isSigned) {
            auto res = ebv0 + ebv1;
            auto axm = implies(ebv0 > zero && ebv1 > zero, res > zero) &&
                       implies(ebv0 < zero && ebv1 < zero, res < zero);
            return res.withAxiom(axm);
        } else {
            ebv0 = ebv0.zgrowTo(sz + 1);
            ebv1 = ebv1.zgrowTo(sz + 1);

            auto res = ebv0 + ebv1;
            auto axm = zero_ == res.extract(sz, sz);

            return res.extract(sz - 1, 0).withAxiom(axm);
        }
    }

    template<size_t N>
    static DynBitVectorExpr doit(BitVector<N> bv0, DynBitVectorExpr bv1, bool isSigned) {
        return doit(DynBitVectorExpr{ bv0 }, bv1, isSigned);
    }

    template<size_t N>
    static DynBitVectorExpr doit(DynBitVectorExpr bv0, BitVector<N> bv1, bool isSigned) {
        return doit(bv0, DynBitVectorExpr{ bv1 }, isSigned);
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem>
Bool distinct(z3::context& ctx, const std::vector<Elem>& elems) {
    if (elems.empty()) return Bool::mkConst(ctx, true);

    std::vector<Z3_ast> cast;
    for (const auto& elem : elems) {
        cast.push_back(z3impl::getExpr(elem));
    }

    z3::expr ret = z3::to_expr(ctx, Z3_mk_distinct(ctx, cast.size(), &cast[0]));

    z3::expr axiom = z3impl::getAxiom(elems[0]);
    for (const auto& elem : elems) {
        axiom = z3impl::spliceAxiomsImpl(axiom, z3impl::getAxiom(elem));
    }

    return Bool{ ret, axiom };
}

template<class ...Args>
Bool forAll(
        z3::context& ctx,
        std::function<Bool(Args...)> func
    ) {
    return Bool{ z3impl::forAll(ctx, func) };
}


template <class VT>
Bool forAllConst(z3::context& ctx, VT var, Bool body) {
    return Bool{ z3impl::forAllConst(ctx, var, body) };
}

template<class ...Args>
Bool forAll(
        z3::context& ctx,
        std::function<Bool(Args...)> func,
        std::function<std::vector<SomeExpr>(Args...)> patternGen
    ) {
    return Bool{ z3impl::forAll(ctx, func, patternGen) };
}

////////////////////////////////////////////////////////////////////////////////

template< class >
class Function; // undefined

template<class Res, class ...Args>
class Function<Res(Args...)> : public Expr {
    z3::func_decl inner;
    z3::expr axiomatic;

    template<class ...CArgs>
    inline static z3::expr massAxiomAnd(CArgs... args) {
        static_assert(sizeof...(args) > 0, "Trying to massAxiomAnd zero axioms");
        return z3impl::spliceAxioms(args...);
    }

    static z3::func_decl constructFunc(z3::context& ctx, const std::string& name) {
        const size_t N = sizeof...(Args);
        z3::sort domain[N] = { impl::generator<Args>::sort(ctx)... };

        return ctx.function(name.c_str(), N, domain, impl::generator<Res>::sort(ctx));
    }

    static z3::func_decl constructFreshFunc(z3::context& ctx, const std::string& name) {
        const size_t N = sizeof...(Args);
        Z3_sort domain[N] = { impl::generator<Args>::sort(ctx)... };
        auto fd = Z3_mk_fresh_func_decl(ctx, name.c_str(), N, domain, impl::generator<Res>::sort(ctx));

        return z3::func_decl(ctx, fd);
    }

    static z3::expr constructAxiomatic(
            z3::context& ctx,
            z3::func_decl z3func,
            std::function<Res(Args...)> realfunc) {

        std::function<Bool(Args...)> lam = [=](Args... args) -> Bool {
            return Bool(z3func(z3impl::getExpr(args)...) == z3impl::getExpr(realfunc(args...)));
        };

        return z3impl::forAll<Bool, Args...>(ctx, lam);
    }

public:

    typedef Function Self;

    Function(const Function&) = default;
    Function(z3::func_decl inner, z3::expr axiomatic):
        inner(inner), axiomatic(axiomatic) {};
    explicit Function(z3::func_decl inner):
        inner(inner), axiomatic(z3impl::defaultAxiom(inner.ctx())) {};
    Function(z3::context& ctx, const std::string& name, std::function<Res(Args...)> f):
        inner(constructFunc(ctx, name)), axiomatic(constructAxiomatic(ctx, inner, f)){}

    z3::func_decl get() const { return inner; }
    z3::expr axiom() const { return axiomatic; }
    z3::context& ctx() const { return inner.ctx(); }

    Self withAxiom(const ValueExpr& ax) const {
        return Self{ inner, z3impl::spliceAxiomsImpl(axiom(), z3impl::asAxiom(ax)) };
    }

    Res operator()(Args... args) const {
        return Res(inner(z3impl::getExpr(args)...), z3impl::spliceAxiomsImpl(axiom(), massAxiomAnd(args...)));
    }

    static z3::sort range(z3::context& ctx) {
        return impl::generator<Res>::sort(ctx);
    }

    template<size_t N = 0>
    static z3::sort domain(z3::context& ctx) {
        return impl::generator< util::index_in_row_q<N, Args...> >::sort(ctx);
    }

    static Self mkFunc(z3::context& ctx, const std::string& name) {
        return Self{ constructFunc(ctx, name) };
    }

    static Self mkFunc(z3::context& ctx, const std::string& name, std::function<Res(Args...)> body) {
        z3::func_decl f = constructFunc(ctx, name);
        z3::expr ax = constructAxiomatic(ctx, f, body);
        return Self{ f, ax };
    }

    static Self mkFreshFunc(z3::context& ctx, const std::string& name) {
        return Self{ constructFreshFunc(ctx, name) };
    }

    static Self mkFreshFunc(z3::context& ctx, const std::string& name, std::function<Res(Args...)> body) {
        z3::func_decl f = constructFreshFunc(ctx, name);
        z3::expr ax = constructAxiomatic(ctx, f, body);
        return Self{ f, ax };
    }

    static Self mkDerivedFunc(
            z3::context& ctx,
            const std::string& name,
            const Self& oldFunc,
            std::function<Res(Args...)> body) {
        z3::func_decl f = constructFreshFunc(ctx, name);
        z3::expr ax = constructAxiomatic(ctx, f, body);
        return Self{ f, z3impl::spliceAxiomsImpl(oldFunc.axiom(), ax) };
    }

    static Self mkDerivedFunc(
            z3::context& ctx,
            const std::string& name,
            const std::vector<Self>& oldFuncs,
            std::function<Res(Args...)> body) {
        z3::func_decl f = constructFreshFunc(ctx, name);

        std::vector<z3::expr> axs;
        axs.reserve(oldFuncs.size() + 1);
        std::transform(oldFuncs.begin(), oldFuncs.end(), std::back_inserter(axs),
            [](const Self& oldFunc) { return oldFunc.axiom(); });
        axs.push_back(constructAxiomatic(ctx, f, body));

        return Self{ f, z3impl::spliceAxiomsImpl(axs) };
    }
};

template<class Res, class ...Args>
std::ostream& operator<<(std::ostream& ost, Function<Res(Args...)> f) {
    return ost << f.get() << " assuming " << f.axiom();
}

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class FuncArray {
    typedef FuncArray<Elem, Index> Self;
    typedef Function<Elem(Index)> inner_t;

    std::shared_ptr<std::string> name;
    inner_t inner;

    FuncArray(const std::string& name, inner_t inner):
        name(std::make_shared<std::string>(name)), inner(inner) {};
    FuncArray(std::shared_ptr<std::string> name, inner_t inner):
        name(name), inner(inner) {};

public:

    FuncArray(const FuncArray&) = default;
    FuncArray(z3::context& ctx, const std::string& name):
        FuncArray(name, inner_t::mkFreshFunc(ctx, name)) {}
    FuncArray(z3::context& ctx, const std::string& name, std::function<Elem(Index)> f):
        FuncArray(name, inner_t::mkFreshFunc(ctx, name, f)) {}

    Elem select    (Index i) const { return inner(i);  }
    Elem operator[](Index i) const { return select(i); }

    Self store(Index i, Elem e) {
        inner_t nf = inner_t::mkDerivedFunc(ctx(), *name, inner, [=](Index j) {
            return if_(j == i).then_(e).else_(this->select(j));
        });

        return Self{ name, nf };
    }

    Self store(const std::vector<std::pair<Index, Elem>>& entries) {
        inner_t nf = inner_t::mkDerivedFunc(ctx(), *name, inner, [=](Index j) {
            return switch_(j, entries, this->select(j));
        });

        return Self{ name, nf };
    }

    z3::context& ctx() const { return inner.ctx(); }

    static Self mkDefault(z3::context& ctx, const std::string& name, Elem def) {
        return Self{ ctx, name, [def](Index){ return def; } };
    }

    static Self mkFree(z3::context& ctx, const std::string& name) {
        return Self{ ctx, name };
    }

    static Self merge(
            const std::string& name,
            Self defaultArray,
            const std::vector<std::pair<Bool, Self>>& arrays) {

        std::vector<inner_t> inners;
        inners.reserve(arrays.size() + 1);
        std::transform(arrays.begin(), arrays.end(), std::back_inserter(inners),
            [](const std::pair<Bool, Self>& e) { return e.second.inner; });
        inners.push_back(defaultArray.inner);

        inner_t nf = inner_t::mkDerivedFunc(defaultArray.ctx(), name, inners,
            [=](Index j) -> Elem {
                std::vector<std::pair<Bool, Elem>> selected;
                selected.reserve(arrays.size());
                std::transform(arrays.begin(), arrays.end(), std::back_inserter(selected),
                    [&j](const std::pair<Bool, Self>& e) {
                        return std::make_pair(e.first, e.second.select(j));
                    }
                );

                return switch_(selected, defaultArray.select(j));
            }
        );

        return Self{ name, nf };
    }

    friend std::ostream& operator<<(std::ostream& ost, const Self& fa) {
        return ost << "funcArray " << *fa.name << " {" << fa.inner << "}";
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class TheoryArray: public ValueExpr {

public:

    TheoryArray(z3::expr inner) : ValueExpr(inner) {
        ASSERT(inner.is_array(), "TheoryArray constructed from non-array");
    };
    TheoryArray(z3::expr inner, z3::expr axioms) : ValueExpr(inner, axioms) {
        ASSERT(inner.is_array(), "TheoryArray constructed from non-array");
    };
    TheoryArray(const TheoryArray&) = default;

    Elem select    (Index i) const { return Elem(z3::select(z3impl::getExpr(this), z3impl::getExpr(i)));  }
    Elem operator[](Index i) const { return select(i); }

    TheoryArray store(Index i, Elem e) {
        return z3::store(z3impl::getExpr(this), z3impl::getExpr(i), z3impl::getExpr(e));
    }

    TheoryArray store(const std::vector<std::pair<Index, Elem>>& entries) {
        z3::expr base = z3impl::getExpr(this);
        for (const auto& entry : entries) {
            base = z3::store(base, z3impl::getExpr(entry.first), z3impl::getExpr(entry.second));
        }
        return base;
    }

    z3::context& ctx() const { return z3impl::getContext(this); }

    static TheoryArray mkDefault(z3::context& ctx, const std::string&, Elem def) {
        return z3::const_array(impl::generator<Index>::sort(ctx), z3impl::getExpr(def));
    }

    static TheoryArray mkFree(z3::context& ctx, const std::string& name) {
        return z3::to_expr(ctx, Z3_mk_fresh_const(
            ctx,
            name.c_str(),
            ctx.array_sort(
                impl::generator<Index>::sort(ctx),
                impl::generator<Elem>::sort(ctx)
            )
        ));
    }

    static TheoryArray merge(
            const std::string&,
            TheoryArray defaultArray,
            const std::vector<std::pair<Bool, TheoryArray>>& arrays) {
        return switch_(arrays, defaultArray);
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class InlinedFuncArray {
    typedef InlinedFuncArray<Elem, Index> Self;
    typedef std::function<Elem(Index)> inner_t;

    z3::context* context;
    std::shared_ptr<std::string> name;
    inner_t inner;

    InlinedFuncArray(
            z3::context& ctx,
            std::shared_ptr<std::string> name,
            inner_t inner
    ) : context(&ctx), name(name), inner(inner) {};

public:

    InlinedFuncArray(const InlinedFuncArray&) = default;
    InlinedFuncArray(z3::context& ctx, const std::string& name, std::function<Elem(Index)> f):
        context(&ctx), name(std::make_shared<std::string>(name)), inner(f) {}
    InlinedFuncArray(z3::context& ctx, const std::string& name):
        context(&ctx), name(std::make_shared<std::string>(name)) {

        // XXX akhin this is as fucked up as before, but also works for now

        auto initial = Function<Elem(Index)>::mkFreshFunc(ctx, "(initial)" + name);
        inner = [initial,&ctx](Index ix) -> Elem {
            return initial(ix);
        };
    }

    Elem select    (Index i) const { return inner(i);  }
    Elem operator[](Index i) const { return select(i); }

    InlinedFuncArray& operator=(const InlinedFuncArray&) = default;

    Self store(Index i, Elem e) {
        inner_t old = this->inner;
        inner_t nf = [=](Index j) {
            return if_(j == i).then_(e).else_(old(j));
        };

        return Self{ *context, name, nf };
    }

    InlinedFuncArray<Elem, Index> store(const std::vector<std::pair<Index, Elem>>& entries) {
        inner_t old = this->inner;
        inner_t nf = [=](Index j) {
            return switch_(j, entries, old(j));
        };

        return Self{ *context, name, nf };
    }

    z3::context& ctx() const { return *context; }

    static Self mkDefault(z3::context& ctx, const std::string& name, Elem def) {
        return Self{ ctx, name, [def](Index){ return def; } };
    }

    static Self mkFree(z3::context& ctx, const std::string& name) {
        return Self{ ctx, name };
    }

    static Self merge(
            const std::string& name,
            Self defaultArray,
            const std::vector<std::pair<Bool, Self>>& arrays) {

        inner_t nf = [=](Index j) -> Elem {
            std::vector<std::pair<Bool, Elem>> selected;
            selected.reserve(arrays.size());
            std::transform(arrays.begin(), arrays.end(), std::back_inserter(selected),
                [&j](const std::pair<Bool, Self>& e) {
                    return std::make_pair(e.first, e.second.select(j));
                }
            );

            return switch_(selected, defaultArray.select(j));
        };

        return Self{ defaultArray.ctx(), name, nf };
    }

    friend std::ostream& operator<<(std::ostream& ost, const Self& ifa) {
        return ost << "inlinedArray " << *ifa.name << " {...}";
    }
};

////////////////////////////////////////////////////////////////////////////////

template<size_t ElemSize = 8, size_t N>
std::vector<BitVector<ElemSize>> splitBytes(BitVector<N> bv) {
    typedef BitVector<ElemSize> Byte;

    if (N <= ElemSize) {
        return std::vector<Byte>{ grow<ElemSize>(bv) };
    }

    z3::context& ctx = z3impl::getContext(bv);

    std::vector<Byte> ret;
    for (size_t ix = 0; ix < N; ix += ElemSize) {
        z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+ElemSize-1, ix, z3impl::getExpr(bv)));
        ret.push_back(Byte{ e, z3impl::getAxiom(bv) });
    }
    return ret;
}

template<size_t ElemSize = 8>
std::vector<BitVector<ElemSize>> splitBytes(SomeExpr bv) {
    typedef BitVector<ElemSize> Byte;

    auto bv_ = bv.to<DynBitVectorExpr>();
    ASSERT(!bv_.empty(), "Non-vector");

    DynBitVectorExpr dbv = bv_.getUnsafe();
    size_t width = dbv.getBitSize();

    if (width <= ElemSize) {
        SomeExpr newbv = dbv.growTo(ElemSize);
        auto byte = newbv.to<Byte>();
        ASSERT(!byte.empty(), "Invalid dynamic BitVector, cannot convert to Byte");
        return std::vector<Byte>{ byte.getUnsafe() };
    }

    z3::context& ctx = z3impl::getContext(dbv);

    std::vector<Byte> ret;
    for (size_t ix = 0; ix < width; ix += ElemSize) {
        z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+ElemSize-1, ix, z3impl::getExpr(dbv)));
        ret.push_back(Byte{ e, z3impl::getAxiom(bv) });
    }
    return ret;
}

template<size_t N, size_t ElemSize = 8>
BitVector<N> concatBytes(const std::vector<BitVector<ElemSize>>& bytes) {
    typedef BitVector<ElemSize> Byte;

    using borealis::util::toString;

    ASSERT(bytes.size() * ElemSize == N,
           "concatBytes failed to merge the resulting BitVector: "
           "expected vector of size " + toString(N/ElemSize) +
           ", got vector of size " + toString(bytes.size()));

    z3::expr head = z3impl::getExpr(bytes[0]);
    z3::context& ctx = head.ctx();

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = z3::expr(ctx, Z3_mk_concat(ctx, z3impl::getExpr(bytes[i]), head));
    }

    z3::expr axiom = z3impl::getAxiom(bytes[0]);
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = z3impl::spliceAxiomsImpl(z3impl::getAxiom(bytes[i]), axiom);
    }

    return BitVector<N>{ head, axiom };
}

template<size_t ElemSize = 8>
SomeExpr concatBytesDynamic(const std::vector<BitVector<ElemSize>>& bytes, size_t bitSize) {
    typedef BitVector<ElemSize> Byte;

    using borealis::util::toString;

    z3::expr head = z3impl::getExpr(bytes[0]);
    z3::context& ctx = head.ctx();

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = z3::expr(ctx, Z3_mk_concat(ctx, z3impl::getExpr(bytes[i]), head));
    }

    z3::expr axiom = z3impl::getAxiom(bytes[0]);
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = z3impl::spliceAxiomsImpl(z3impl::getAxiom(bytes[i]), axiom);
    }

    return DynBitVectorExpr{ head, axiom }.adapt(bitSize);
}

////////////////////////////////////////////////////////////////////////////////

template<class Index, size_t ElemSize = 8, template<class, class> class InnerArray = TheoryArray>
class ScatterArray {
    typedef BitVector<ElemSize> Byte;
    typedef InnerArray<Byte, Index> Inner;

    Inner inner;

    ScatterArray(InnerArray<Byte, Index> inner): inner(inner) {};

public:

    ScatterArray(const ScatterArray&) = default;
    ScatterArray(ScatterArray&&) = default;
    ScatterArray& operator=(const ScatterArray&) = default;
    ScatterArray& operator=(ScatterArray&&) = default;

    SomeExpr select(Index i, size_t elemBitSize) const {
        std::vector<Byte> bytes;
        for (size_t j = 0; j <= (elemBitSize - 1)/ElemSize; ++j) {
            bytes.push_back(inner[i+j]);
        }
        return concatBytesDynamic(bytes, elemBitSize);
    }

    template<class Elem>
    Elem select(Index i) const {
        enum{ elemBitSize = Elem::bitsize };

        std::vector<Byte> bytes;
        for (size_t j = 0; j <= (elemBitSize - 1)/ElemSize; ++j) {
            bytes.push_back(inner[i+j]);
        }
        return concatBytes<elemBitSize>(bytes);
    }

    z3::context& ctx() const { return inner.ctx(); }

    Byte operator[](Index i) const {
        return inner[i];
    }

    Byte operator[](long long i) const {
        return inner[Index::mkConst(ctx(), i)];
    }

    template<class Elem>
    inline ScatterArray store(Index i, Elem e, size_t elemBitSize) {
        std::vector<Byte> bytes = splitBytes<ElemSize>(e);

        std::vector<std::pair<Index, Byte>> cases;
        for (size_t j = 0; j <= (elemBitSize - 1)/ElemSize; ++j) {
            cases.push_back({ i+j, bytes[j] });
        }
        return inner.store(cases);
    }

    template<class Elem, GUARD(Elem::bitsize >= 0)>
    ScatterArray store(Index i, Elem e) {
        return store(i, e, Elem::bitsize);
    }

    ScatterArray store(Index i, SomeExpr e) {
        auto bv = e.to<DynBitVectorExpr>();
        ASSERTC(!bv.empty());
        auto bitsize = bv.getUnsafe().getBitSize();
        return store(i, e, bitsize);
    }

    static ScatterArray mkDefault(z3::context& ctx, const std::string& name, Byte def) {
        return ScatterArray{ Inner::mkDefault(ctx, name, def) };
    }

    static ScatterArray mkFree(z3::context& ctx, const std::string& name) {
        return ScatterArray{ Inner::mkFree(ctx, name) };
    }

    static ScatterArray merge(
            const std::string& name,
            ScatterArray defaultArray,
            const std::vector<std::pair<Bool, ScatterArray>>& arrays
        ) {

        std::vector<std::pair<Bool, Inner>> inners;
        inners.reserve(arrays.size());
        std::transform(arrays.begin(), arrays.end(), std::back_inserter(inners),
            [](const std::pair<Bool, ScatterArray>& p) {
                return std::make_pair(p.first, p.second.inner);
            }
        );

        Inner merged = Inner::merge(name, defaultArray.inner, inners);

        return ScatterArray{ merged };
    }

    friend std::ostream& operator<<(std::ostream& ost, const ScatterArray& arr) {
        return ost << arr.inner;
    }
};

#undef ASPECT
#undef ASPECT_END
#undef ASPECT_BEGIN

} // namespace logic
} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"

#endif // Z3_LOGIC_HPP

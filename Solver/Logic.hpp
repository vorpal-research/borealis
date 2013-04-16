/*
 * Logic.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: belyaev
 */

#ifndef LOGIC_HPP
#define LOGIC_HPP

#include <z3/z3++.h>

#include <functional>

#include "Type/TypeFactory.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {
namespace logic {

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
    inline z3::expr spliceAxioms(z3::expr e0, z3::expr e1) {
        return (e0 && e1).simplify();
    }
} // namespace z3impl

////////////////////////////////////////////////////////////////////////////////

class ValueExpr;

namespace z3impl {
    z3::expr getExpr(const ValueExpr& a);
    z3::expr getAxiom(const ValueExpr& a);
    z3::sort getSort(const ValueExpr& a);
    z3::context& getContext(const ValueExpr& a);
    z3::expr asAxiom(const ValueExpr& e);

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
    inline z3::expr asAxiom(const ValueExpr* e) {
        ASSERTC(e != nullptr); return asAxiom(*e);
    }
} // namespace z3impl

class ValueExpr: public Expr {
    struct Impl;
    Impl* pimpl;

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

    void swap(ValueExpr&);
};

template<class Expr0, class Expr1>
inline z3::expr spliceAxioms(const Expr0& e0, const Expr1& e1) {
    return z3impl::spliceAxioms(
        z3impl::getAxiom(e0), z3impl::getAxiom(e1)
    );
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
\
        static CLASS mkConst(z3::context& ctx, typename impl::generator<CLASS>::basetype value) { \
            return CLASS(impl::generator<CLASS>::mkConst(ctx, value)); \
        } \
\
        static CLASS mkBound(z3::context& ctx, unsigned i) { \
            return CLASS(ctx, Z3_mk_bound(ctx, i, impl::generator<CLASS>::sort(ctx))); \
        } \
\
        static CLASS mkVar(z3::context& ctx, const std::string& name) { \
            return CLASS(ctx.constant(name.c_str(), impl::generator<CLASS>::sort(ctx))); \
        } \
\
        static CLASS mkVar( \
                z3::context& ctx, \
                const std::string& name, \
                std::function<Bool(CLASS)> daAxiom) { \
            /* first construct the no-axiom version */ \
            CLASS nav = mkVar(ctx, name); \
            return CLASS(z3impl::getExpr(nav), \
                         z3impl::spliceAxioms( \
                                 z3impl::getAxiom(nav), \
                                 z3impl::asAxiom(daAxiom(nav))) \
            ); \
        } \
\
        static CLASS mkFreshVar(z3::context& ctx, const std::string& name) { \
            return CLASS(ctx, Z3_mk_fresh_const(ctx, name.c_str(), impl::generator<CLASS>::sort(ctx))); \
        } \
\
        static CLASS mkFreshVar( \
                z3::context& ctx, \
                const std::string& name, \
                std::function<Bool(CLASS)> daAxiom) { \
            /* first construct the no-axiom version */ \
            CLASS nav = mkFreshVar(ctx, name); \
            return CLASS( \
                    z3impl::getExpr(nav), \
                    z3impl::spliceAxioms(z3impl::getAxiom(nav), z3impl::asAxiom(daAxiom(nav))) \
            ); \
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

    static z3::sort sort(z3::context& ctx) { return ctx.bv_sort(N)  ; }
    static bool check(z3::expr e) { return e.is_bv() && e.get_sort().bv_size() == N; }
    static z3::expr mkConst(z3::context& ctx, int n) { return ctx.bv_val(n, N); }
};
} // namespace impl

template<size_t N>
ASPECT_BEGIN(BitVector)
    enum{ bitsize = N };
    size_t getBitSize() const { return bitsize; }
ASPECT_END

template<size_t N0, size_t N1>
inline
typename std::enable_if<(N0 == N1), BitVector<N1>>::type
grow(BitVector<N1> bv) {
    return bv;
}

template<size_t N0, size_t N1>
inline
typename std::enable_if<(N0 > N1), BitVector<N0>>::type
grow(BitVector<N1> bv) {
    z3::context& ctx = z3impl::getContext(bv);

    return BitVector<N0>(
        z3::to_expr(ctx, Z3_mk_sign_ext(ctx, N0-N1, z3impl::getExpr(bv))),
        z3impl::getAxiom(bv)
    );
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
            return Bool(z3impl::getExpr(ebv0) OP z3impl::getExpr(ebv1), spliceAxioms(ebv0, ebv1)); \
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
            return BitVector<M>(z3impl::getExpr(ebv0) OP z3impl::getExpr(ebv1), spliceAxioms(ebv0, ebv1)); \
        }


REDEF_BV_BIN_OP(+)
REDEF_BV_BIN_OP(-)
REDEF_BV_BIN_OP(*)
REDEF_BV_BIN_OP(/)
REDEF_BV_BIN_OP(|)
REDEF_BV_BIN_OP(&)
REDEF_BV_BIN_OP(>>)
REDEF_BV_BIN_OP(<<)
REDEF_BV_BIN_OP(^)

#undef REDEF_BV_BIN_OP


template<size_t N0, size_t N1, size_t M = impl::max(N0, N1)>
BitVector<M> operator %(BitVector<N0> bv0, BitVector<N1> bv1) {
    auto ebv0 = grow<M>(bv0);
    auto ebv1 = grow<M>(bv1);
    auto& ctx = z3impl::getContext(bv0);

    auto res = z3::to_expr(ctx, Z3_mk_bvsmod(ctx, z3impl::getExpr(ebv0), z3impl::getExpr(ebv1)));
    return BitVector<M>(res, spliceAxioms(ebv0, ebv1));
}


#define REDEF_BV_INT_BIN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(BitVector<N> bv, int v1) { \
            return BitVector<N>(z3impl::getExpr(bv) OP v1, z3impl::getAxiom(bv)); \
        }


REDEF_BV_INT_BIN_OP(+)
REDEF_BV_INT_BIN_OP(-)
REDEF_BV_INT_BIN_OP(*)
REDEF_BV_INT_BIN_OP(/)

#undef REDEF_BV_INT_BIN_OP


#define REDEF_INT_BV_BIN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(int v1, BitVector<N> bv) { \
            return BitVector<N>(v1 OP z3impl::getExpr(bv), z3impl::getAxiom(bv)); \
        }


REDEF_INT_BV_BIN_OP(+)
REDEF_INT_BV_BIN_OP(-)
REDEF_INT_BV_BIN_OP(*)
REDEF_INT_BV_BIN_OP(/)

#undef REDEF_INT_BV_BIN_OP


#define REDEF_UN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(BitVector<N> bv) { \
            return BitVector<N>(OP z3impl::getExpr(bv), z3impl::getAxiom(bv)); \
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
            return typename merger<E, E2>::type(
                   z3::to_expr(
                       ctx,
                       Z3_mk_ite(
                           ctx,
                           z3impl::getExpr(cond),
                           z3impl::getExpr(tb),
                           z3impl::getExpr(fb)
                       )
                   ),
                   z3impl::spliceAxioms(
                       z3impl::getAxiom(cond),
                       z3::to_expr(
                           ctx,
                           Z3_mk_ite(
                               ctx,
                               z3impl::getExpr(cond),
                               z3impl::getAxiom(tb),
                               z3impl::getAxiom(fb)
                           )
                       )
                   )
            );
        }
    };

    struct thener {
        Bool cond;

        template<class E>
        inline elser<E> then_(E tbranch) {
            return elser<E> { cond, tbranch };
        }
    };

    thener operator()(Bool cond) {
        return thener {cond};
    }
};

inline ifer::thener if_(Bool cond) {
    return ifer()(cond);
}

template<class T, class U>
T switch_(
        U val,
        const std::vector<std::pair<U, T>>& cases,
        T default_
    ) {

    auto mkIte = [val](T b, const std::pair<U, T>& a) {
        return if_(val == a.first)
               .then_(a.second)
               .else_(b);
    };

    return std::accumulate(cases.begin(), cases.end(), default_, mkIte);
}

////////////////////////////////////////////////////////////////////////////////

namespace impl {

template<class Tl, size_t ...N>
std::tuple<typename util::index_in_type_list<N, Tl>::type...>
mkbounds_step_1(z3::context& ctx, Tl, util::indexer<N...>) {
    using namespace borealis::util;

    return std::tuple<typename util::index_in_type_list<N, Tl>::type...> {
        util::index_in_type_list<N, Tl>::type::mkBound(ctx, N)...
    };
}

} // namespace impl

template<class ...Args>
std::tuple<Args...> mkBounds(z3::context& ctx) {
    using namespace borealis::util;
    return impl::mkbounds_step_1(ctx, type_list<Args...>(), typename make_indexer<Args...>::type());
}

////////////////////////////////////////////////////////////////////////////////

namespace z3impl {

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
    auto body = util::apply_packed(func, bounds);

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
                    nullptr,
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
            return DynBitVectorExpr(
                    z3::to_expr(ctx,
                            Z3_mk_sign_ext(ctx, n-m, z3impl::getExpr(this))
                    ),
                    z3impl::getAxiom(this)
            );
        else return DynBitVectorExpr(z3impl::getExpr(this), z3impl::getAxiom(this));
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

////////////////////////////////////////////////////////////////////////////////

// Untyped logic expression
class SomeExpr: public ValueExpr {
public:
    typedef SomeExpr self;

    SomeExpr(z3::expr e): ValueExpr(e) {};
    SomeExpr(z3::expr e, z3::expr axiom): ValueExpr(e, axiom) {};
    SomeExpr(const SomeExpr&) = default;
    SomeExpr(SomeExpr&&) = default;
    SomeExpr(const ValueExpr& b): ValueExpr(b) {};

    static SomeExpr mkDynamic(Bool b) { return SomeExpr(b); }

    template<size_t N>
    static SomeExpr mkDynamic(BitVector<N> bv) { return SomeExpr(bv); }

    template<class Aspect>
    bool is() {
        return impl::generator<Aspect>::check(z3impl::getExpr(this));
    }

    template<class Aspect>
    util::option<Aspect> to() {
        using util::just;
        using util::nothing;

        if (is<Aspect>()) return just(Aspect(*this));
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

    // equality comparison operators are the most general ones
    Bool operator==(const SomeExpr& that) {
        return Bool(z3impl::getExpr(this) == z3impl::getExpr(that), spliceAxioms(*this, that));
    }

    Bool operator!=(const SomeExpr& that) {
        return Bool(z3impl::getExpr(this) != z3impl::getExpr(that), spliceAxioms(*this, that));
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem>
Bool distinct(z3::context& ctx, const std::vector<Elem>& elems) {
    if (elems.empty()) return Bool::mkConst(ctx, true);

    std::vector<Z3_ast> cast;
    for (auto elem : elems) {
        cast.push_back(z3impl::getExpr(elem));
    }

    z3::expr ret = z3::to_expr(ctx, Z3_mk_distinct(ctx, cast.size(), &cast[0]));

    z3::expr axiom = z3impl::getAxiom(elems[0]);
    for (auto elem : elems) {
        axiom = z3impl::spliceAxioms(axiom, z3impl::getAxiom(elem));
    }

    return Bool{ ret, axiom };
}

template<class ...Args>
Bool forAll(
        z3::context& ctx,
        std::function<Bool(Args...)> func
    ) {
    return Bool(z3impl::forAll(ctx, func));
}

template<class ExprClass>
ExprClass addAxiom(ExprClass expr, Bool axiom) {
    return ExprClass{
        z3impl::getExpr(expr),
        z3impl::spliceAxioms(z3impl::getAxiom(expr), z3impl::asAxiom(axiom))
    };
}

////////////////////////////////////////////////////////////////////////////////

template< class >
class Function; // undefined

template<class Res, class ...Args>
class Function<Res(Args...)> : public Expr {
    z3::func_decl inner;
    z3::expr axiomatic;

    template<class CArg>
    inline static z3::expr massAxiomAnd(CArg arg){
        return z3impl::getAxiom(arg);
    }

    template<class CArg0, class CArg1, class ...CArgs>
    inline static z3::expr massAxiomAnd(CArg0 arg0, CArg1 arg1, CArgs... args) {
        return z3impl::spliceAxioms(z3impl::getAxiom(arg0), massAxiomAnd(arg1, args...));
    }

    static z3::func_decl constructFunc(z3::context& ctx, const std::string& name){
        const size_t N = sizeof...(Args);
        z3::sort domain[N] = { impl::generator<Args>::sort(ctx)... };
        return ctx.function(name.c_str(), N, domain, impl::generator<Res>::sort(ctx));
    }

    static z3::func_decl constructFreshFunc(z3::context& ctx, const std::string& name){
        const size_t N = sizeof...(Args);
        Z3_sort domain[N] = { impl::generator<Args>::sort(ctx)... };
        auto fd = Z3_mk_fresh_func_decl(ctx, name.c_str(), N, domain, impl::generator<Res>::sort(ctx));

        return z3::func_decl(ctx, fd);
    }

    static z3::expr constructAxiomatic(
            z3::context& ctx,
            z3::func_decl z3func,
            std::function<Res(Args...)> realfunc) {

        std::function<Bool(Args...)> lam = [&](Args... args)->Bool {
            return Bool(z3func(z3impl::getExpr(args)...) == z3impl::getExpr(realfunc(args...)));
        };

        return z3impl::forAll<Bool, Args...>(ctx, lam);
    }

public:
    typedef Function self;

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

    Res operator()(Args... args) {
        return Res(inner(z3impl::getExpr(args)...), z3impl::spliceAxioms(this->axiom(), massAxiomAnd(args...)).simplify());
    }

    static z3::sort range(z3::context& ctx) {
        return impl::generator<Res>::sort(ctx);
    }
    template<size_t N = 0>
    static z3::sort domain(z3::context& ctx) {
        return impl::generator<typename borealis::util::index_in_row<N, Args...>::type>::sort(ctx);
    }

    static self mkFunc(z3::context& ctx, const std::string& name) {
        return self(constructFunc(ctx, name));
    }

    static self mkFunc(z3::context& ctx, const std::string& name, std::function<Res(Args...)> body) {
        z3::func_decl f = constructFunc(ctx, name);
        z3::expr ax = constructAxiomatic(ctx, f, body);
        return self(f, ax);
    }

    static self mkFreshFunc(z3::context& ctx, const std::string& name, std::function<Res(Args...)> body) {
        z3::func_decl f = constructFreshFunc(ctx, name);
        z3::expr ax = constructAxiomatic(ctx, f, body);
        return self(f, ax);
    }

    static self mkDerivedFunc(const self& oldFunc, const std::string& name, std::function<Res(Args...)> body) {
        z3::func_decl f = constructFreshFunc(oldFunc.ctx(), name);
        z3::expr ax = constructAxiomatic(oldFunc.ctx(), f, body);
        return self(f, ax && oldFunc.axiom());
    }
};

template<class Res, class ...Args>
std::ostream& operator<<(std::ostream& ost, Function<Res(Args...)> f) {
    return ost << f.get() << " assuming " << f.axiom().simplify();
}

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class FuncArray {
    typedef Function<Elem(Index)> inner_t;

    std::shared_ptr<std::string> name;
    inner_t inner;

    FuncArray(inner_t inner, std::shared_ptr<std::string>& name): name(name), inner(inner) {};

public:
    FuncArray(const FuncArray&) = default;
    FuncArray(z3::context& ctx, const std::string& name, std::function<Elem(Index)> f):
        name(std::make_shared<std::string>(name)), inner(inner_t::mkFreshFunc(ctx, name, f)) {}

    Elem select    (Index i) { return inner(i);  }
    Elem operator[](Index i) { return select(i); }

    FuncArray<Elem, Index> store(Index i, Elem e) {
        inner_t nf = inner_t::mkDerivedFunc(inner, *name, [this,&i,&e](Index res) {
            return if_(res == i).then_(e).else_(this->select(res));
        });

        return FuncArray<Elem, Index> (nf, name);
    }

    FuncArray<Elem, Index> store(const std::vector<std::pair<Index, Elem>>& entries) {
        inner_t nf = inner_t::mkDerivedFunc(inner, *name, [this, entries](Index j) {
            return switch_(j, entries, this->select(j));
        });

        return FuncArray<Elem, Index> (nf, name);
    }

    z3::context& ctx() { return inner.ctx(); }

    static FuncArray mkDefault(z3::context& ctx, const std::string& name, Elem def) {
        return FuncArray(ctx, name, [def](Index){ return def; });
    }

    friend std::ostream& operator<<(std::ostream& ost, const FuncArray<Elem, Index>& fa) {
        return ost << "funcArray " << *fa.name << " {" << fa.inner << "}";
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class InlinedFuncArray {
    typedef std::function<Elem(Index)> inner_t;

    std::shared_ptr<std::string> name;
    inner_t inner;
    z3::context* context;

    InlinedFuncArray(
            z3::context& ctx,
            inner_t inner,
            std::shared_ptr<std::string>& name
    ) : name(name), inner(inner), context(&ctx) {};
public:
    InlinedFuncArray(const InlinedFuncArray&) = default;
    InlinedFuncArray(z3::context& ctx, const std::string& name, std::function<Elem(Index)> f):
        name(std::make_shared<std::string>(name)), inner(f), context(&ctx) {}

    Elem select    (Index i) { return inner(i);  }
    Elem operator[](Index i) { return select(i); }

    InlinedFuncArray& operator=(const InlinedFuncArray& o) {
        this->name = o.name;
        this->inner = o.inner;
        this->context = o.context;
        return *this;
    }

    InlinedFuncArray<Elem, Index> store(Index i, Elem e) {
        inner_t existing = this->inner;
        inner_t nf = [this,&i,&e](Index j) {
            return if_(j == i).then_(e).else_(inner(j));
        };

        return InlinedFuncArray<Elem, Index> (*context, nf, name);
    }

    InlinedFuncArray<Elem, Index> store(const std::vector<std::pair<Index, Elem>>& entries) {
        inner_t existing = this->inner;
        inner_t nf = [this, existing, entries](Index j) {
            return switch_(j, entries, existing(j));
        };

        return InlinedFuncArray<Elem, Index> (*context, nf, name);
    }

    z3::context& ctx() const { return *context; }

    static InlinedFuncArray mkDefault(z3::context& ctx, const std::string& name, Elem def) {
        return InlinedFuncArray(ctx, name, [def](Index){ return def; });
    }

    friend std::ostream& operator<<(std::ostream& ost, const InlinedFuncArray<Elem, Index>& ifa) {
        return ost << "inlinedArray " << *ifa.name << " { ... }";
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class TheoryArray: public ValueExpr {
    TheoryArray(z3::expr inner) : ValueExpr(inner) {
        ASSERT(inner.is_array(), "TheoryArray constructed from non-array");
    };
public:
    TheoryArray(const TheoryArray&) = default;

    Elem select    (Index i) { return Elem(z3::select(z3impl::getExpr(this), z3impl::getExpr(i)));  }
    Elem operator[](Index i) { return select(i); }

    TheoryArray store(Index i, Elem e) {
        return z3::store(z3impl::getExpr(this), z3impl::getExpr(i), z3impl::getExpr(e));
    }

    TheoryArray store(const std::vector<std::pair<Index, Elem>>& entries) {
        z3::expr base = z3impl::getExpr(this);
        for (auto& entry: entries) {
            base = z3::store(base, z3impl::getExpr(entry.first), z3impl::getExpr(entry.second));
        }
        return base;
    }

    z3::context& ctx() const { return z3impl::getContext(this); }

    static TheoryArray mkDefault(z3::context& ctx, const std::string&, Elem def) {
        return z3::const_array(impl::generator<Index>::sort(ctx), z3impl::getExpr(def));
    }

};

////////////////////////////////////////////////////////////////////////////////

template<size_t N, size_t ElemSize = 8>
std::vector<BitVector<ElemSize>> splitBytes(BitVector<N> bv) {
    typedef BitVector<ElemSize> Byte;

    z3::context& ctx = z3impl::getContext(bv);

    std::vector<Byte> ret;

    if (N <= ElemSize) {
        return std::vector<Byte>{ grow<ElemSize>(bv) };
    }

    for (size_t ix = 0; ix < N; ix += ElemSize) {
        z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+ElemSize-1, ix, z3impl::getExpr(bv)));
        ret.push_back(Byte(e, z3impl::getAxiom(bv)));
    }

    return ret;
}

template<size_t ElemSize = 8>
std::vector<BitVector<ElemSize>> splitBytes(SomeExpr bv) {
    typedef BitVector<ElemSize> Byte;

    ASSERT(bv.is<DynBitVectorExpr>(), "Non-vector");

    DynBitVectorExpr bvv = bv.to<DynBitVectorExpr>().getUnsafe();
    size_t width = bvv.getBitSize();

    if (width <= ElemSize) {
        SomeExpr newv = bvv.growTo(ElemSize);
        ASSERT(newv.is<Byte>(), "Invalid dynamic BitVector, cannot convert to Byte");
        return std::vector<Byte>{ newv.to<Byte>().getUnsafe() };
    }

    z3::context& ctx = z3impl::getContext(bvv);

    std::vector<Byte> ret;

    for (size_t ix = 0; ix < width; ix += ElemSize) {
        z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+ElemSize-1, ix, z3impl::getExpr(bvv)));
        ret.push_back(Byte(e, z3impl::getAxiom(bv)));
    }

    return ret;
}

template<size_t N, size_t ElemSize = 8>
BitVector<N> concatBytes(const std::vector<BitVector<ElemSize>>& bytes) {
    typedef BitVector<ElemSize> Byte;

    using borealis::util::toString;

    ASSERT(bytes.size() * ElemSize == N,
           "concatBytes failed to merge the resulting BitVector: "
           "expected vector size " + toString(N/ElemSize) + ", got vector of size " +
           toString(bytes.size()));

    z3::expr head = z3impl::getExpr(bytes[0]);
    z3::context& ctx = head.ctx();

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = z3::expr(ctx, Z3_mk_concat(ctx, z3impl::getExpr(bytes[i]), head));
    }

    z3::expr axiom = z3impl::getAxiom(bytes[0]);
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = z3impl::spliceAxioms(axiom, z3impl::getAxiom(bytes[i]));
    }

    return BitVector<N>(head, axiom);
}

template<size_t ElemSize = 8>
SomeExpr concatBytesDynamic(const std::vector<BitVector<ElemSize>>& bytes) {
    typedef BitVector<ElemSize> Byte;

    using borealis::util::toString;

    z3::expr head = z3impl::getExpr(bytes[0]);
    z3::context& ctx = head.ctx();

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = z3::expr(ctx, Z3_mk_concat(ctx, z3impl::getExpr(bytes[i]), head));
    }

    z3::expr axiom = z3impl::getAxiom(bytes[0]);
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = z3impl::spliceAxioms(axiom, z3impl::getAxiom(bytes[i]));
    }

    return SomeExpr(head, axiom);
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

    SomeExpr select(Index i, size_t elemBitSize) {
        std::vector<Byte> bytes;
        for (size_t j = 0; j < elemBitSize; j+=ElemSize) {
            bytes.push_back(inner[i+j/ElemSize]);
        }
        return concatBytesDynamic(bytes);
    }

    template<class Elem>
    Elem select(Index i) {
        enum { elemBitSize = Elem::bitsize };

        std::vector<Byte> bytes;
        for (int j = 0; j < elemBitSize; j+=ElemSize) {
            bytes.push_back(inner[i+j/ElemSize]);
        }
        return concatBytes<elemBitSize>(bytes);
    }

    z3::context& ctx() { return inner.ctx(); }

    Byte operator[](Index i) {
        return inner[i];
    }

    Byte operator[](long long i) {
        return inner[Index::mkConst(ctx(), i)];
    }

    template<class Elem>
    inline ScatterArray store(Index i, Elem e, size_t elemBitSize) {
        std::vector<Byte> bytes = splitBytes<ElemSize>(e);

        std::vector<std::pair<Index, Byte>> cases;
        for (auto j = 0U; j < elemBitSize/ElemSize; ++j) {
            cases.push_back({ i+j, bytes[j] });
        }

        return inner.store(cases);
    }

    template<class Elem>
    ScatterArray store(Index i, Elem e) {
        return store(i, e, Elem::bitsize);
    }

    ScatterArray store(Index i, SomeExpr e) {
        ASSERTC(e.is<DynBitVectorExpr>());

        auto bv = e.to<DynBitVectorExpr>().getUnsafe();
        return store(i, e, bv.getBitSize());
    }

    static ScatterArray mkDefault(z3::context& ctx, const std::string& name, Byte def) {
        return ScatterArray{ Inner::mkDefault(ctx, name, def) };
    }
};

} // namespace logic
} // namespace borealis

#include "Util/unmacros.h"

#endif // LOGIC_HPP

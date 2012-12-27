/*
 * Logic.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: belyaev
 */

#ifndef LOGIC_HPP
#define LOGIC_HPP

#include <functional>

#include <z3/z3++.h>

#include "Util/util.h"

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

class ValueExpr: public Expr {
    struct Impl;
    Impl* pimpl;

public:
    ValueExpr(const ValueExpr&);
    ValueExpr(ValueExpr&&);
    ValueExpr(z3::expr e, z3::expr axiom);
    ValueExpr(z3::expr e);
    ~ValueExpr();

    ValueExpr& operator=(const ValueExpr&);

    z3::expr get() const;
    z3::expr axiom() const;
    z3::sort get_sort() const;
    z3::context& ctx() const;

    void swap(ValueExpr&);
};

template<class Expr0, class Expr1>
inline z3::expr spliceAxioms(const Expr0& e0, const Expr1& e1) {
    return z3impl::spliceAxioms(e0.axiom(), e1.axiom());
}

////////////////////////////////////////////////////////////////////////////////

class Bool: public ValueExpr {
public:
    typedef Bool self;

    Bool(const Bool&);
    Bool(z3::expr e, z3::expr axiom);
    Bool(z3::expr e);
private:
    Bool(z3::context& ctx, Z3_ast ast);
public:
    Bool& operator=(const Bool&) = default;

    Bool implies(Bool that) const;
    Bool iff(Bool that) const;
    z3::expr toAxiom() const;
    static z3::sort sort(z3::context& ctx);
    static self mkBound(z3::context& ctx, unsigned i);
    static self mkConst(z3::context& ctx, bool value);
    static self mkVar(z3::context& ctx, const std::string& name);
    static self mkVar(
            z3::context& ctx,
            const std::string& name,
            std::function<Bool(Bool)> daAxiom);
    static self mkFreshVar(z3::context& ctx, const std::string& name);
    static self mkFreshVar(
            z3::context& ctx,
            const std::string& name,
            std::function<Bool(Bool)> daAxiom);
};

std::ostream& operator<<(std::ostream& ost, Bool b);

#define REDEF_BOOL_BOOL_OP(OP) \
        Bool operator OP(Bool bv0, Bool bv1);

REDEF_BOOL_BOOL_OP(==)
REDEF_BOOL_BOOL_OP(!=)
REDEF_BOOL_BOOL_OP(&&)
REDEF_BOOL_BOOL_OP(||)

#undef REDEF_BOOL_BOOL_OP

Bool operator!(Bool bv0);

////////////////////////////////////////////////////////////////////////////////

template<size_t N>
class BitVector: public ValueExpr {

public:
    typedef BitVector self;
    enum{ bitsize = N };

    BitVector(const self&) = default;
    BitVector(z3::expr e, z3::expr axiom) : ValueExpr(e, axiom) {
        if(!(e.is_bv() && e.get_sort().bv_size() == N)) {
            throw ConversionException(
                    "Trying to construct bitvector<" +
                    borealis::util::toString(N) +
                    "> from: " +
                    borealis::util::toString(e)
            );
        }
    }
    explicit BitVector(z3::expr e) : ValueExpr(e) {
        if(!(e.is_bv() && e.get_sort().bv_size() == N)) {
            throw ConversionException(
                    "Trying to construct bitvector<" +
                    borealis::util::toString(N) +
                    "> from: " +
                    borealis::util::toString(e)
            );
        }
    }
private:
    BitVector(z3::context& ctx, Z3_ast ast) : BitVector(z3::to_expr(ctx, ast)){}
public:
    self& operator=(const self&) = default;

    inline size_t getBitSize() const {
        return bitsize;
    }

    static z3::sort sort(z3::context& ctx) {
        return ctx.bv_sort(N);
    }

    static self mkBound(z3::context& ctx, unsigned i) {
        return self(ctx, Z3_mk_bound(ctx, i, sort(ctx)));
    }

    static self mkConst(z3::context& ctx, long long value) {
        return self(ctx.bv_val(value, N));
    }

    static self mkVar(z3::context& ctx, const std::string& name) {
        return self(ctx.bv_const(name.c_str(), N));
    }

    static self mkVar(
            z3::context& ctx,
            const std::string& name,
            std::function<Bool(BitVector<N>)> daAxiom) {
        // first construct the no-axiom version
        self nav = mkVar(ctx, name);
        return self(nav.get(), z3impl::spliceAxioms(nav.axiom(), daAxiom(nav).toAxiom()));
    }

    static self mkFreshVar(z3::context& ctx, const std::string& name) {
        return self(ctx, Z3_mk_fresh_const(ctx, name.c_str(), sort(ctx)));
    }

    static self mkFreshVar(
            z3::context& ctx,
            const std::string& name,
            std::function<Bool(Bool)> daAxiom) {
        // first construct the no-axiom version
        self nav = mkFreshVar(ctx, name);
        return self(nav.get(), z3impl::spliceAxioms(nav.axiom(), daAxiom(nav).toAxiom()));
    }
};



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
    z3::context& ctx = bv.get().ctx();

    return BitVector<N0>(
        z3::to_expr(ctx, Z3_mk_sign_ext(ctx, N0-N1, bv.get())),
        bv.axiom()
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
            return Bool(ebv0.get() OP ebv1.get(), spliceAxioms(ebv0, ebv1)); \
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
            return BitVector<M>(ebv0.get() OP ebv1.get(), spliceAxioms(ebv0, ebv1)); \
        }


REDEF_BV_BIN_OP(+)
REDEF_BV_BIN_OP(-)
REDEF_BV_BIN_OP(*)
REDEF_BV_BIN_OP(/)

#undef REDEF_BV_BIN_OP


#define REDEF_BV_INT_BIN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(BitVector<N> bv, long long v1) { \
            return BitVector<N>(bv.get() OP v1, bv.axiom()); \
        }


REDEF_BV_INT_BIN_OP(+)
REDEF_BV_INT_BIN_OP(-)
REDEF_BV_INT_BIN_OP(*)
REDEF_BV_INT_BIN_OP(/)

#undef REDEF_BV_INT_BIN_OP


#define REDEF_INT_BV_BIN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(long long v1, BitVector<N> bv) { \
            return BitVector<N>(v1 OP bv.get(), bv.axiom()); \
        }


REDEF_INT_BV_BIN_OP(+)
REDEF_INT_BV_BIN_OP(-)
REDEF_INT_BV_BIN_OP(*)
REDEF_INT_BV_BIN_OP(/)

#undef REDEF_INT_BV_BIN_OP


template<size_t N>
std::ostream& operator<<(std::ostream& ost, BitVector<N> bv) {
    return ost << bv.get().simplify() << " assuming " << bv.axiom().simplify();
}

////////////////////////////////////////////////////////////////////////////////

struct ifer {
    template<class E>
    struct elser {
        Bool cond;
        E tbranch;

        template<class E2>
        inline typename merger<E,E2>::type else_(E2 fbranch) {
            auto& ctx = cond.get().ctx();
            auto tb = merger<E,E2>::app(tbranch);
            auto fb = merger<E,E2>::app(fbranch);
            return typename merger<E, E2>::type(
                   z3::to_expr(
                       ctx,
                       Z3_mk_ite(
                           ctx,
                           cond.get(),
                           tb.get(),
                           fb.get()
                       )
                   ),
                   z3impl::spliceAxioms(
                       cond.axiom(),
                       z3::to_expr(
                           ctx,
                           Z3_mk_ite(
                               ctx,
                               cond.get(),
                               tb.axiom(),
                               fb.axiom()
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
    std::vector<z3::sort> sorts { Args::sort(ctx)... };

    size_t numBounds = sorts.size();

    auto bounds = mkBounds<Args...>(ctx);
    auto body = util::apply_packed(func, bounds);

    std::vector<Z3_sort> sort_array(sorts.rbegin(), sorts.rend());

    std::vector<Z3_symbol> name_array;
    for(size_t i = 0U; i < numBounds; ++i) {
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
                    body.get()));
    return axiom;
}

} // namespace z3impl

////////////////////////////////////////////////////////////////////////////////

class ComparableExpr: public ValueExpr {
public:
    typedef ComparableExpr self;

    ComparableExpr(z3::expr e, z3::expr axiom):
        ValueExpr(e, axiom) {
        if(!e.is_arith() && !e.is_bv()) {
            throw ConversionException("ComparableExpr constructed from incompatible expression: " +
                    borealis::util::toString(e));
        }
    }

#define REDEF_OP(OP) \
    Bool operator OP(const ComparableExpr& that) { \
        return Bool(get() OP that.get(), spliceAxioms(*this, that)); \
    }

    REDEF_OP(<)
    REDEF_OP(>)
    REDEF_OP(>=)
    REDEF_OP(<=)

#undef REDEF_OP

};

class DynBitVectorExpr: public ValueExpr {
public:
    typedef DynBitVectorExpr self;

    DynBitVectorExpr(z3::expr e, z3::expr axiom):
        ValueExpr(e, axiom) {
        if(!e.is_bv()) {
            throw ConversionException("BitVectorExpr constructed from incompatible expression: " +
                    borealis::util::toString(e));
        }
    }

    size_t getBitSize() const {
        return get_sort().bv_size();
    }

};

template<class T>
struct isBoolT: public std::integral_constant<bool, false> {};

template<>
struct isBoolT<Bool>: public std::integral_constant<bool, true> {};

template<class T>
struct isBitVectorT: public std::integral_constant<bool, false> {};

template<size_t N>
struct isBitVectorT<BitVector<N>>: public std::integral_constant<bool, true> {};

template<class T>
struct isComparableExpr: public std::integral_constant<bool, false> {};

template<>
struct isComparableExpr<ComparableExpr>: public std::integral_constant<bool, true> {};

////////////////////////////////////////////////////////////////////////////////

// untyped logic expression
class SomeExpr: public ValueExpr {
public:
    typedef SomeExpr self;

    SomeExpr(z3::expr e): ValueExpr(e) {};
    SomeExpr(z3::expr e, z3::expr axiom): ValueExpr(e, axiom) {};
    SomeExpr(const SomeExpr&) = default;
    SomeExpr(SomeExpr&&) = default;
    SomeExpr(Bool b): ValueExpr(b) {};
    template<size_t N>
    SomeExpr(BitVector<N> bv): ValueExpr(bv) {};

    static SomeExpr mkDynamic(Bool b) { return SomeExpr(b); }

    template<size_t N>
    static SomeExpr mkDynamic(BitVector<N> bv) { return SomeExpr(bv); }

    bool isBool() {
        return get().get_sort().is_bool();
    }

    borealis::util::option<Bool> toBool() {
        if(isBool())
            return borealis::util::just(Bool(get(), axiom()));
        else
            return borealis::util::nothing<Bool>();
    }

    template<size_t N>
    bool isBitVector() {
        return get().get_sort().is_bv() && get().get_sort().bv_size() == N;
    }

    template<size_t N>
    borealis::util::option<BitVector<N>> toBitVector() {
        if(isBitVector<N>())
            return borealis::util::just(BitVector<N>(get(), axiom()));
        else
            return borealis::util::nothing<BitVector<N>>();
    }

    bool isComparable() {
        return get().is_arith() || get().is_bv();
    }

    borealis::util::option<ComparableExpr> toComparable() {
        if(isComparable())
            return borealis::util::just(ComparableExpr(get(), axiom()));
        else
            return borealis::util::nothing<ComparableExpr>();
    }

    template<class To>
    borealis::util::option<To> to(typename std::enable_if<isBoolT<To>::value>::type* = nullptr) {
        return toBool();
    }

    template<class To>
    borealis::util::option<To> to(typename std::enable_if<isBitVectorT<To>::value>::type* = nullptr) {
        return toBitVector<To::bitsize>();
    }

    template<class To>
    borealis::util::option<To> to(typename std::enable_if<isComparableExpr<To>::value>::type* = nullptr) {
        return toComparable();
    }

    template<class To>
    bool is(typename std::enable_if<isBoolT<To>::value>::type* = nullptr) {
        return isBool();
    }

    template<class To>
    bool is(typename std::enable_if<isBitVectorT<To>::value>::type* = nullptr) {
        return isBitVector<To::bitsize>();
    }

    template<class To>
    bool is(typename std::enable_if<isComparableExpr<To>::value>::type* = nullptr) {
        return isComparable();
    }

    // equality comparison operators are the most general ones
    Bool operator==(const SomeExpr& that) {
        return Bool(this->get() == that.get(), spliceAxioms(*this, that));
    }

    Bool operator!=(const SomeExpr& that) {
        return Bool(this->get() != that.get(), spliceAxioms(*this, that));
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem>
Bool distinct(z3::context& ctx, const std::vector<Elem> elems) {
    if (elems.empty()) return Bool::mkConst(ctx, true);

    std::vector<Z3_ast> cast;
    for (auto elem : elems) {
        cast.push_back(elem.get());
    }

    z3::expr ret = z3::to_expr(ctx, Z3_mk_distinct(ctx, cast.size(), &cast[0]));

    z3::expr axiom = elems[0].axiom();
    for(auto elem: elems) {
        axiom = z3impl::spliceAxioms(axiom, elem.axiom());
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

////////////////////////////////////////////////////////////////////////////////

template< class >
class Function; // undefined

template<class Res, class ...Args>
class Function<Res(Args...)> : public Expr {
    z3::func_decl inner;
    z3::expr axiomatic;

    template<class CArg>
    inline static z3::expr massAxiomAnd(CArg arg){
        return arg.axiom();
    }

    template<class CArg0, class CArg1, class ...CArgs>
    inline static z3::expr massAxiomAnd(CArg0 arg0, CArg1 arg1, CArgs... args) {
        return z3impl::spliceAxioms(arg0.axiom(), massAxiomAnd(arg1, args...));
    }

    static z3::func_decl constructFunc(z3::context& ctx, const std::string& name){
        const size_t N = sizeof...(Args);
        z3::sort domain[N] = { Args::sort(ctx)... };
        return ctx.function(name.c_str(), N, domain, Res::sort(ctx));
    }

    static z3::func_decl constructFreshFunc(z3::context& ctx, const std::string& name){
        const size_t N = sizeof...(Args);
        Z3_sort domain[N] = { Args::sort(ctx)... };
        auto fd = Z3_mk_fresh_func_decl(ctx, name.c_str(), N, domain, Res::sort(ctx));

        return z3::func_decl(ctx, fd);
    }

    static z3::expr constructAxiomatic(
            z3::context& ctx,
            z3::func_decl z3func,
            std::function<Res(Args...)> realfunc) {

        std::function<Bool(Args...)> lam = [&](Args... args)->Bool{
            return Bool(z3func(args.get()...) == realfunc(args...).get());
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
        return Res(inner(args.get()...), massAxiomAnd(*this, args...).simplify());
    }

    static z3::sort range(z3::context& ctx) {
        return Res::sort(ctx);
    }
    template<size_t N = 0>
    static z3::sort domain(z3::context& ctx) {
        return borealis::util::index_in_row<N, Args...>::type::sort(ctx);
    }

    static self mkFunc(z3::context& ctx, const std::string& name) {
        return self(constructFunc(ctx, name));
    }

    static self mkFunc(z3::context& ctx, const std::string& name, std::function<Res(Args...)> body) {
        z3::func_decl f = constructFunc(ctx, name);
        z3::expr ax = constructAxiomatic(ctx, f , body);
        return self(f, ax);
    }

    static self mkFreshFunc(z3::context& ctx, const std::string& name, std::function<Res(Args...)> body) {
        z3::func_decl f = constructFreshFunc(ctx, name);
        z3::expr ax = constructAxiomatic(ctx, f , body);
        return self(f, ax);
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

    FuncArray(inner_t inner, std::shared_ptr<std::string>& name): name(name), inner(inner){};
public:
    FuncArray(const FuncArray&) = default;
    FuncArray(z3::context& ctx, const std::string& name, std::function<Elem(Index)> f):
        name(std::make_shared<std::string>(name)), inner(inner_t::mkFreshFunc(ctx, name, f)){}

    Elem select    (Index i) { return inner(i);  }
    Elem operator[](Index i) { return select(i); }

    FuncArray<Elem, Index> store(Index i, Elem e) {
        inner_t nf = inner_t::mkFreshFunc(inner.get().ctx(), *name, [this](Index res) {
            return if_(res == i).then_(e).else_(this->select(res));
        });

        return FuncArray<Elem, Index> (nf, name);
    }

    FuncArray<Elem, Index> store(std::vector<std::pair<Index, Elem>> entries) {
        inner_t nf = inner_t::mkFreshFunc(inner.get().ctx(), *name, [this, entries](Index j) {
            return switch_(j, entries, this->select(j));
        });

        return FuncArray<Elem, Index> (nf, name);
    }

    z3::context& ctx() { return inner.ctx(); }

    static FuncArray mkDefault(z3::context& ctx, const std::string& name, Elem def) {
        return FuncArray(ctx, name, [def](Index){ return def; });
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class InlinedFuncArray {
    typedef std::function<Elem(Index)> inner_t;

    std::shared_ptr<std::string> name;
    inner_t inner;
    z3::context& context;

    InlinedFuncArray(
            z3::context& ctx,
            inner_t inner,
            std::shared_ptr<std::string>& name
    ) : name(name), inner(inner), context(ctx){};
public:
    InlinedFuncArray(const InlinedFuncArray&) = default;
    InlinedFuncArray(z3::context& ctx, const std::string& name, std::function<Elem(Index)> f):
        name(std::make_shared<std::string>(name)), inner(f), context(ctx){}

    Elem select    (Index i) { return inner(i);  }
    Elem operator[](Index i) { return select(i); }

    InlinedFuncArray<Elem, Index> store(Index i, Elem e) {
        inner_t existing = this->inner;
        inner_t nf = [this, existing](Index j) {
            return if_(j == i).then_(e).else_(inner(j));
        };

        return InlinedFuncArray<Elem, Index> (context, nf, name);
    }

    InlinedFuncArray<Elem, Index> store(std::vector<std::pair<Index, Elem>> entries) {
        inner_t existing = this->inner;
        inner_t nf = [this, existing, entries](Index j) {
            return switch_(j, entries, existing(j));
        };

        return InlinedFuncArray<Elem, Index> (context, nf, name);
    }

    z3::context& ctx() const { return context; }

    static InlinedFuncArray mkDefault(z3::context& ctx, const std::string& name, Elem def) {
        return InlinedFuncArray(ctx, name, [def](Index){ return def; });
    }
};

////////////////////////////////////////////////////////////////////////////////

template<size_t N, size_t ElemSize = 8>
std::vector<BitVector<ElemSize>> splitBytes(BitVector<N> bv) {
    typedef BitVector<ElemSize> Byte;

    z3::context& ctx = bv.get().ctx();

    std::vector<Byte> ret;

    if(N <= ElemSize) {
        return std::vector<Byte>{ grow<ElemSize>(bv) };
    }

    for (size_t ix = 0; ix < N; ix += ElemSize) {
        z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+ElemSize-1, ix, bv.get()));
        ret.push_back(Byte(e, bv.axiom()));
    }

    return ret;
}

template<size_t ElemSize = 8>
std::vector<BitVector<ElemSize>> splitBytes(SomeExpr bv) {
    typedef BitVector<ElemSize> Byte;

    size_t width = bv.get().get_sort().bv_size();

    // FIXME: check for the <= case
    if(width == ElemSize) {
        for(auto& ibv: bv.to<Byte>()) {
            return std::vector<Byte>{ ibv };
        }

        return
           util::sayonara<std::vector<Byte>>(
                   __FILE__,
                   __LINE__,
                   __PRETTY_FUNCTION__,
                   "Invalid dynamic bitvector, cannot convert to Byte");
    }

    z3::context& ctx = bv.get().ctx();

    std::vector<Byte> ret;

    for (size_t ix = 0; ix < width; ix += ElemSize) {
        z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+ElemSize-1, ix, bv.get()));
        ret.push_back(Byte(e, bv.axiom()));
    }

    return ret;
}

template<size_t N, size_t ElemSize = 8>
BitVector<N> concatBytes(const std::vector<BitVector<ElemSize>>& bytes) {
    typedef BitVector<ElemSize> Byte;

    using borealis::util::toString;

    if (bytes.size() * ElemSize != N) throw ConversionException {
        "concatBytes failed to merge the resulting BitVector: "
        "expected vector size " + toString(N/ElemSize) + ", got vector of size "
        + toString(bytes.size())
    };

    z3::expr head = bytes[0].get();
    z3::context& ctx = head.ctx();

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = z3::expr(ctx, Z3_mk_concat(ctx, bytes[i].get(), head));
    }

    z3::expr axiom = bytes[0].axiom();
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = z3impl::spliceAxioms(axiom, bytes[i].axiom());
    }

    return BitVector<N>(head, axiom);
}

template<size_t ElemSize = 8>
SomeExpr concatBytesDynamic(const std::vector<BitVector<ElemSize>>& bytes) {
    typedef BitVector<ElemSize> Byte;

    using borealis::util::toString;

    z3::expr head = bytes[0].get();
    z3::context& ctx = head.ctx();

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = z3::expr(ctx, Z3_mk_concat(ctx, bytes[i].get(), head));
    }

    z3::expr axiom = bytes[0].axiom();
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = z3impl::spliceAxioms(axiom, bytes[i].axiom());
    }

    return SomeExpr(head, axiom);
}

////////////////////////////////////////////////////////////////////////////////

template<class Index, size_t ElemSize = 8, template<class, class> class InnerArray = FuncArray>
class ScatterArray {
    typedef BitVector<ElemSize> Byte;
    typedef InnerArray<Byte, Index> Inner;

    Inner inner;

    ScatterArray(InnerArray<Byte, Index> inner): inner(inner){};

public:
    ScatterArray(const ScatterArray&) = default;
    ScatterArray(ScatterArray&&) = default;
    ScatterArray& operator=(const ScatterArray&) = default;

    SomeExpr select(Index i, size_t elemBitSize) {
        std::vector<Byte> bytes;
        for(size_t j = 0; j < elemBitSize; j+=ElemSize) {
            bytes.push_back(inner[i+j/ElemSize]);
        }
        return concatBytesDynamic(bytes);
    }

    template<class Elem>
    Elem select(Index i) {
        enum { elemBitSize = Elem::bitsize };

        std::vector<Byte> bytes;
        for(int j = 0; j < elemBitSize; j+=ElemSize) {
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
        for(auto j = 0U; j < elemBitSize/ElemSize; ++j) {
            cases.push_back({ i+j, bytes[j] });
        }

        return inner.store(cases);
    }

    template<class Elem>
    ScatterArray store(Index i, Elem e) {
        return store(i, e, Elem::bitsize);
    }

    ScatterArray store(Index i, SomeExpr e) {
        return store(i, e,  e.get().get_sort().bv_size());
    }

    static ScatterArray mkDefault(z3::context& ctx, const std::string& name, Byte def) {
        return ScatterArray{ Inner(ctx, name, [def](Index){ return def; }) };
    }
};

} // namespace logic
} // namespace borealis

#endif // LOGIC_HPP

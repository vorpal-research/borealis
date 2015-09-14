/*
 * Logic.hpp
 *
 *  Created on: Jul 31, 2013
 *      Author: Sam Kolton
 */

#ifndef MSAT_LOGIC_HPP_
#define MSAT_LOGIC_HPP_

#include <functional>
#include <vector>

#include "SMT/MathSAT/MathSAT.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {
namespace logic {

class Expr {};

namespace msatimpl {
    inline mathsat::Expr defaultAxiom(const mathsat::Env& env) {
        return env.bool_val(true);
    }
    inline mathsat::Expr defaultAxiom(const mathsat::Expr& e) {
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
    const mathsat::Env& getEnvironment(const ValueExpr& a);
    std::string getName(const ValueExpr& e);
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
    inline const mathsat::Env& getEnvironment(const ValueExpr* a) {
        ASSERTC(a != nullptr); return getEnvironment(*a);
    }
    inline std::string getName(const ValueExpr* e) {
        ASSERTC(e != nullptr); return getName(*e);
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
    friend const mathsat::Env& msatimpl::getEnvironment(const ValueExpr& a);
    friend std::string msatimpl::getName(const ValueExpr& a);

    void swap(ValueExpr&);

    std::string getName() const;
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

    static mathsat::Sort sort(mathsat::Env& env) { return env.bv_sort(N); }
    static bool check(mathsat::Expr e) { return e.is_bv() && e.get_sort().bv_size() == N; }
    static mathsat::Expr mkConst(mathsat::Env& env, int n) { return env.bv_val(n, N); }
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
    mathsat::Env env = msatimpl::getEnvironment(bv);

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
REDEF_BV_BIN_OP(%)

#undef REDEF_BV_BIN_OP


#define REDEF_BV_INT_BIN_OP(OP) \
        template<size_t N> \
        BitVector<N> operator OP(BitVector<N> bv, int v1) { \
            return BitVector<N>{ \
                msatimpl::getExpr(bv) OP v1, \
                msatimpl::getAxiom(bv) \
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
                v1 OP msatimpl::getExpr(bv), \
                msatimpl::getAxiom(bv) \
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
                OP msatimpl::getExpr(bv), \
                msatimpl::getAxiom(bv) \
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
            auto tb = merger<E,E2>::app(tbranch);
            auto fb = merger<E,E2>::app(fbranch);
            return typename merger<E, E2>::type{
                mathsat::ite(
                    msatimpl::getExpr(cond),
                    msatimpl::getExpr(tb),
                    msatimpl::getExpr(fb)
                ),
                msatimpl::spliceAxioms(cond, tb, fb)
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

class ComparableExpr;

namespace impl {
template<>
struct generator<ComparableExpr> : generator<BitVector<1>> {
    static bool check(mathsat::Expr e) { return e.is_bool() || e.is_bv() || e.is_int() || e.is_rat(); }
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

class UComparableExpr;

namespace impl {
template<>
struct generator<UComparableExpr> : generator<BitVector<1>> {
    static bool check(mathsat::Expr e) { return e.is_bv(); }
};
} // namespace impl

ASPECT_BEGIN(UComparableExpr)
public:

#define DEF_OP(NAME) \
Bool NAME(const UComparableExpr& other) { \
    auto l = msatimpl::getExpr(this); \
    auto r = msatimpl::getExpr(other); \
    auto res = mathsat::NAME(l, r); \
    auto axm = msatimpl::spliceAxioms(*this, other); \
    return Bool{ res, axm }; \
}

DEF_OP(ugt)
DEF_OP(uge)
DEF_OP(ult)
DEF_OP(ule)

#undef DEF_OP

ASPECT_END

////////////////////////////////////////////////////////////////////////////////

class DynBitVectorExpr;

namespace impl {
template<>
struct generator<DynBitVectorExpr> : generator<BitVector<1>> {
    static bool check(mathsat::Expr e) { return e.is_bv(); }
};
} // namespace impl

ASPECT_BEGIN(DynBitVectorExpr)
public:
    size_t getBitSize() const {
        return msatimpl::getSort(this).bv_size();
    }

    DynBitVectorExpr growTo(size_t n) const {
        size_t m = getBitSize();
        if (m < n)
            return DynBitVectorExpr{
                mathsat::sext(msatimpl::getExpr(this), n-m),
                msatimpl::getAxiom(this)
            };
        else return DynBitVectorExpr{ *this };
    }

    DynBitVectorExpr zgrowTo(size_t n) const {
        size_t m = getBitSize();
        if (m < n)
            return DynBitVectorExpr{
                mathsat::zext(msatimpl::getExpr(this), n-m),
                msatimpl::getAxiom(this)
            };
        else return DynBitVectorExpr{ *this };
    }

    DynBitVectorExpr extract(size_t high, size_t low) const {
        size_t m = getBitSize();
        ASSERT(high < m, "High must be less then bit-vector size.");
        ASSERT(low <= high, "Low mustn't be greater then high.");
        return DynBitVectorExpr{
            mathsat::extract(msatimpl::getExpr(this), high, low),
            msatimpl::getAxiom(this)
        };
    }

    DynBitVectorExpr adapt(size_t N) const {
        if (N > getBitSize()) {
            return zgrowTo(N);
        } else if (N < getBitSize()) {
            return extract(N - 1, 0);
        } else {
            return *this;
        }
    }

    DynBitVectorExpr lshr(const DynBitVectorExpr& shift) {
        size_t sz = std::max(getBitSize(), shift.getBitSize());
        DynBitVectorExpr w = this->growTo(sz);
        DynBitVectorExpr s = shift.growTo(sz);

        auto res = mathsat::lshr(msatimpl::getExpr(w), msatimpl::getExpr(s));
        auto axm = msatimpl::spliceAxioms(w, s);
        return DynBitVectorExpr{ res, axm };
    }

    static DynBitVectorExpr mkSizedConst(mathsat::Env& env, int value, size_t bitSize) {
        return DynBitVectorExpr{ env.bv_val(value, bitSize) };
    }

    static DynBitVectorExpr mkSizedVar(mathsat::Env& env, const std::string& name, size_t bitSize) {
        return DynBitVectorExpr{ env.bv_const(name, bitSize) };
    }

    static DynBitVectorExpr mkFreshSizedVar(mathsat::Env& env, const std::string& name, size_t bitSize) {
        return DynBitVectorExpr{ env.fresh_constant(name, env.bv_sort(bitSize)) };
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

// Untyped logic expression
class SomeExpr: public ValueExpr {
public:
    typedef SomeExpr self;

    SomeExpr(mathsat::Expr e): ValueExpr(e) {};
    SomeExpr(mathsat::Expr e, mathsat::Expr axiom): ValueExpr(e, axiom) {};
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
        return impl::generator<Aspect>::check(msatimpl::getExpr(this));
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

    bool isUnsignedComparable() {
        return is<UComparableExpr>();
    }

    borealis::util::option<UComparableExpr> toUnsignedComparable() {
        return to<UComparableExpr>();
    }

    borealis::util::option<ComparableExpr> toComparable() {
        return to<ComparableExpr>();
    }

    // equality comparison operators are the most general ones
    Bool operator==(const SomeExpr& that) {
        return Bool{
            msatimpl::getExpr(this) == msatimpl::getExpr(that),
            msatimpl::spliceAxioms(*this, that)
        };
    }

    Bool operator!=(const SomeExpr& that) {
        return Bool{
            msatimpl::getExpr(this) != msatimpl::getExpr(that),
            msatimpl::spliceAxioms(*this, that)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem>
Bool distinct(mathsat::Env& env, const std::vector<Elem>& elems) {
    if (elems.empty()) return Bool::mkConst(env, true);

    std::vector<mathsat::Expr> cast;
    for (const auto& elem : elems) {
        cast.push_back(msatimpl::getExpr(elem));
    }

    auto ret = mathsat::distinct(cast);

    auto axiom = msatimpl::getAxiom(elems[0]);
    for (const auto& elem : elems) {
        axiom = msatimpl::spliceAxioms(axiom, msatimpl::getAxiom(elem));
    }

    return Bool{ ret, axiom };
}

////////////////////////////////////////////////////////////////////////////////

template< class >
class Function; // undefined

template<class Res, class ...Args>
class Function<Res(Args...)> : public Expr {
    mathsat::Decl inner;
    mathsat::Expr axiomatic;

    template<class ...CArgs>
    inline static mathsat::Expr massAxiomAnd(CArgs... args) {
        static_assert(sizeof...(args) > 0, "Trying to massAxiomAnd zero axioms");
        return msatimpl::spliceAxioms({ msatimpl::getAxiom(args)... });
    }

    static mathsat::Decl constructFunc(mathsat::Env& env, const std::string& name) {
        std::vector<mathsat::Sort> domain = { impl::generator<Args>::sort(env)... };
        return env.function(name.c_str(), domain, impl::generator<Res>::sort(env));
    }

    static mathsat::Decl constructFreshFunc(mathsat::Env& env, const std::string& name) {
        std::vector<mathsat::Sort> domain = { impl::generator<Args>::sort(env)... };
        return env.fresh_function(name.c_str(), domain, impl::generator<Res>::sort(env));
    }

public:

    typedef Function Self;

    Function(const Function&) = default;
    Function(mathsat::Decl inner, mathsat::Expr axiomatic):
        inner(inner), axiomatic(axiomatic) {};
    explicit Function(mathsat::Decl inner):
        inner(inner), axiomatic(msatimpl::defaultAxiom(inner.env())) {};

    mathsat::Decl get() const { return inner; }
    mathsat::Expr axiom() const { return axiomatic; }
    mathsat::Env& env() const { return inner.env(); }

    Res operator()(Args... args) const {
        const std::vector<mathsat::Expr> vec_args = {msatimpl::getExpr(args)...};
        return Res(inner(vec_args), msatimpl::spliceAxioms(axiom(), massAxiomAnd(args...)));
    }

    static mathsat::Sort range(mathsat::Env& env) {
        return impl::generator<Res>::sort(env);
    }

    template<size_t N = 0>
    static mathsat::Sort domain(mathsat::Env& env) {
        return impl::generator< util::index_in_row_q<N, Args...> >::sort(env);
    }

    static Self mkFunc(mathsat::Env& env, const std::string& name) {
        return Self{ constructFunc(env, name) };
    }

    static Self mkFreshFunc(mathsat::Env& env, const std::string& name) {
        return Self{ constructFreshFunc(env, name) };
    }
};

template<class Res, class ...Args>
std::ostream& operator<<(std::ostream& ost, Function<Res(Args...)> f) {
    return ost << f.get() << " assuming " << f.axiom();
}

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class InlinedFuncArray {
    typedef InlinedFuncArray<Elem, Index> Self;
    typedef std::function<Elem(Index)> inner_t;

    mathsat::Env* environment;
    std::shared_ptr<std::string> name;
    inner_t inner;

    InlinedFuncArray(
            mathsat::Env& env,
            std::shared_ptr<std::string> name,
            inner_t inner
    ) : environment(&env), name(name), inner(inner) {};

public:

    InlinedFuncArray(const InlinedFuncArray&) = default;
    InlinedFuncArray(mathsat::Env& env, const std::string& name, std::function<Elem(Index)> f):
        environment(&env), name(std::make_shared<std::string>(name)), inner(f) {}
    InlinedFuncArray(mathsat::Env& env, const std::string& name):
        environment(&env), name(std::make_shared<std::string>(name)) {

        // XXX akhin this is as fucked up as before, but also works for now

        auto initial = Function<Elem(Index)>::mkFreshFunc(env, "(initial)" + name);
        inner = [initial,&env](Index ix) -> Elem {
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

        return Self{ *environment, name, nf };
    }

    InlinedFuncArray<Elem, Index> store(const std::vector<std::pair<Index, Elem>>& entries) {
        inner_t old = this->inner;
        inner_t nf = [=](Index j) {
            return switch_(j, entries, old(j));
        };

        return Self{ *environment, name, nf };
    }

    mathsat::Env& env() const { return *environment; }

    static Self mkDefault(mathsat::Env& env, const std::string& name, Elem def) {
        return Self{ env, name, [def](Index){ return def; } };
    }

    static Self mkFree(mathsat::Env& env, const std::string& name) {
        return Self{ env, name };
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

        return Self{ defaultArray.env(), name, nf };
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

    std::vector<Byte> ret;
    for (size_t ix = 0; ix < N; ix += ElemSize) {
        mathsat::Expr e = mathsat::extract(msatimpl::getExpr(bv), ix+ElemSize-1, ix);
        ret.push_back(Byte{ e, msatimpl::getAxiom(bv) });
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

    std::vector<Byte> ret;
    for (size_t ix = 0; ix < width; ix += ElemSize) {
        mathsat::Expr e = mathsat::extract(msatimpl::getExpr(dbv), ix+ElemSize-1, ix);
        ret.push_back(Byte{ e, msatimpl::getAxiom(bv) });
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

    mathsat::Expr head = msatimpl::getExpr(bytes[0]);

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = mathsat::concat(msatimpl::getExpr(bytes[i]), head);
    }

    mathsat::Expr axiom = msatimpl::getAxiom(bytes[0]);
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = msatimpl::spliceAxioms(msatimpl::getAxiom(bytes[i]), axiom);
    }

    return BitVector<N>{ head, axiom };
}

template<size_t ElemSize = 8>
SomeExpr concatBytesDynamic(const std::vector<BitVector<ElemSize>>& bytes, size_t bitSize) {
    typedef BitVector<ElemSize> Byte;

    using borealis::util::toString;

    mathsat::Expr head = msatimpl::getExpr(bytes[0]);

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = mathsat::concat(msatimpl::getExpr(bytes[i]), head);
    }

    mathsat::Expr axiom = msatimpl::getAxiom(bytes[0]);
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = msatimpl::spliceAxioms(msatimpl::getAxiom(bytes[i]), axiom);
    }

    return DynBitVectorExpr{ head, axiom }.adapt(bitSize);
}

////////////////////////////////////////////////////////////////////////////////

template<class Index, size_t ElemSize = 8, template<class, class> class InnerArray = InlinedFuncArray>
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

    mathsat::Env& env() const { return inner.env(); }

    Byte operator[](Index i) const {
        return inner[i];
    }

    Byte operator[](long long i) const {
        return inner[Index::mkConst(env(), i)];
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

    static ScatterArray mkDefault(mathsat::Env& env, const std::string& name, Byte def) {
        return ScatterArray{ Inner::mkDefault(env, name, def) };
    }

    static ScatterArray mkFree(mathsat::Env& env, const std::string& name) {
        return ScatterArray{ Inner::mkFree(env, name) };
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
} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"

#endif  //MSAT_LOGIC_HPP_

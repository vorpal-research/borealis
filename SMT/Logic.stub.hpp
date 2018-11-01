#include "Util/macros.h"

namespace borealis {
namespace NAMESPACE {
using smt = ENGINE;
namespace logic {

static borealis::config::BoolConfigEntry useProactiveSimplify("z3", "aggressive-simplify");

class Expr {};

namespace smt_impl {
inline smt::expr_t defaultAxiom(smt::context_t& ctx) {
    return smt::mkBoolConst(ctx, true);
}
inline smt::expr_t spliceAxiomsImpl(smt::context_t& ctx, smt::expr_t e0, smt::expr_t e1) {
    if(smt::term_equality(ctx, e0, defaultAxiom(ctx))) return e1;
    if(smt::term_equality(ctx, e1, defaultAxiom(ctx))) return e0;
    return smt::conjunction(ctx, e0, e1);
}
inline smt::expr_t spliceAxiomsImpl(smt::context_t& ctx, smt::expr_t e0, smt::expr_t e1, smt::expr_t e2) {
    if(smt::term_equality(ctx, e0, defaultAxiom(ctx))) return spliceAxiomsImpl(ctx, e1, e2);
    if(smt::term_equality(ctx, e1, defaultAxiom(ctx))) return spliceAxiomsImpl(ctx, e0, e2);
    if(smt::term_equality(ctx, e2, defaultAxiom(ctx))) return spliceAxiomsImpl(ctx, e0, e1);
    return smt::conjunction(ctx, e0, e1, e2);
}
inline smt::expr_t spliceAxiomsImpl(smt::context_t& ctx, std::initializer_list<smt::expr_t> il) {
    return smt::conjunctionOfCollection(ctx, il);
}
inline smt::expr_t spliceAxiomsImpl(smt::context_t& ctx, const std::vector<smt::expr_t>& v) {
    return smt::conjunctionOfCollection(ctx, v);
}
} // namespace smt_impl

////////////////////////////////////////////////////////////////////////////////

class ValueExpr;

class ValueExpr: public Expr {
    smt::expr_t inner;
    smt::expr_t axiom;
    smt::context_t* ctx;

public:
    ValueExpr(const ValueExpr&) = default;
    ValueExpr(ValueExpr&&) = default;
    ValueExpr(smt::context_t& ctx, smt::expr_t e, smt::expr_t axiom):
        inner(e), axiom(axiom), ctx(&ctx) {}
    ValueExpr(smt::context_t& ctx, smt::expr_t e):
        inner(e), axiom(smt_impl::defaultAxiom(ctx)), ctx(&ctx) {}
    ~ValueExpr() = default;

    ValueExpr& operator=(const ValueExpr&) = default;
    ValueExpr& operator=(ValueExpr&&) = default;

    smt::expr_t getExpr() const { return inner; }
    smt::expr_t getAxiom() const { return axiom; }
    smt::context_t& getCtx() const { return *ctx; }

    size_t hash() const {
        return util::hash::simple_hash_value(inner, axiom);
    }
    std::string getName() const {
        return smt::name(inner);
    }

    smt::sort_t getSort() const {
        return smt::get_sort(getCtx(), getExpr());
    }

    std::string toSmtLib() const {
        return smt::toSMTLib(getExpr()) + "\n" + smt::toSMTLib(getAxiom());
    }

    static bool eq(const ValueExpr& a, const ValueExpr& b) {
        auto&& ctx = a.getCtx();
        return smt::term_equality(ctx, a.getExpr(), b.getExpr())
               && smt::term_equality(ctx, a.getAxiom(), b.getAxiom());
    }

    ValueExpr simplify() const {
        auto&& ctx = getCtx();
        return ValueExpr(ctx, smt::simplify(ctx, getExpr()), smt::simplify(ctx, getAxiom()));
    }

    smt::expr_t asAxiom() const {
        return smt_impl::spliceAxiomsImpl(getCtx(), getExpr(), getAxiom());
    }

    ValueExpr withAxiom(const ValueExpr& ax) const {
        auto&& ctx = getCtx();
        return ValueExpr(ctx, getExpr(), ax.asAxiom());
    }

    inline bool isValid() const {
        return true;
    }

    inline explicit operator bool() const { return isValid(); }
};

namespace smt_impl {

// FIXME: this overload is genuinly stupid, but optimizes smth
template<class Expr0, class Expr1>
inline smt::expr_t spliceAxioms(const Expr0& e0, const Expr1& e1) {
    return spliceAxiomsImpl(e0.getCtx(), e0.getAxiom(), e1.getAxiom());
};

template<class Expr0, class Expr1, class Expr2>
inline smt::expr_t spliceAxioms(const Expr0& e0, const Expr1& e1, const Expr2& e2) {
    return spliceAxiomsImpl(e0.getCtx(), e0.getAxiom(), e1.getAxiom(), e2.getAxiom());
};

template<class Expr, class ...Exprs>
inline smt::expr_t spliceAxioms(const Expr& e0, const Exprs&... exprs) {
    return spliceAxiomsImpl(
        e0.getCtx(),
        { e0.getAxiom(), exprs.getAxiom()... }
    );
}

} // namespace smt_impl

template<class EExprClass, class AExprClass>
EExprClass addAxiom(const EExprClass& expr, const AExprClass& axiom) {
    auto&& ctx = expr.getCtx();
    return EExprClass{
        ctx,
        expr.getExpr(),
        smt_impl::spliceAxiomsImpl(ctx, expr.getAxiom(), axiom.getExpr(), axiom.getAxiom())
    };
}

inline std::ostream& operator<<(std::ostream& ost, const ValueExpr& v) {
    return ost << v.toSmtLib();
}

////////////////////////////////////////////////////////////////////////////////

namespace impl {
template<class Aspect>
struct generator;
}

////////////////////////////////////////////////////////////////////////////////

#define ASPECT_BEGIN_WITH_BASE(CLASS, BASE)                                                                            \
    class CLASS: public BASE {                                                                                         \
        using traits = impl::generator<CLASS>;                                                                         \
        using basetype = typename traits::basetype;                                                                    \
    public:                                                                                                            \
        CLASS(smt::context_t& ctx, smt::expr_t e): BASE(ctx, e){};                                                     \
        CLASS(smt::context_t& ctx, smt::expr_t e, smt::expr_t a):                                                      \
            BASE(ctx, e, a){};                                                                                         \
        CLASS(const CLASS&) = default;                                                                                 \
        CLASS(CLASS&&) = default;                                                                                      \
        CLASS(const ValueExpr& e): BASE(e){};                                                                          \
        CLASS& operator=(const CLASS&) = default;                                                                      \
        CLASS simplify() const { return CLASS{ BASE::simplify() }; }                                                   \
        CLASS withAxiom(const ValueExpr& axiom) const {                                                                \
            return addAxiom(*this, axiom);                                                                             \
        }                                                                                                              \
        static inline smt::sort_t getStaticSort(smt::context_t& ctx) {                                                 \
            return traits::sort(ctx);                                                                                  \
        }                                                                                                              \
        static inline CLASS forceCast(const ValueExpr& ve) {                                                           \
            auto&& ctx = ve.getCtx();                                                                                  \
            return CLASS{ ctx, traits::convert(ctx, ve.getExpr()), ve.getAxiom() };                                    \
        }                                                                                                              \
        static inline CLASS forceCast(smt::context_t&, const CLASS& ve) {                                              \
            return ve;                                                                                                 \
        }                                                                                                              \
        static CLASS mkConst(smt::context_t& ctx, basetype value) {                                                    \
            return CLASS{ ctx, traits::mkConst(ctx, value) };                                                          \
        }                                                                                                              \
        static CLASS mkBound(smt::context_t& ctx, unsigned i) {                                                        \
            return CLASS{ ctx, smt::mkBound(ctx, i, getStaticSort(ctx)) };                                             \
        }                                                                                                              \
        static CLASS mkVar(smt::context_t& ctx, const std::string& name) {                                             \
            return CLASS{                                                                                              \
                ctx,                                                                                                   \
                smt::mkVar(ctx, getStaticSort(ctx), name, SmtEngine::freshness::normal)                                \
            };                                                                                                         \
        }                                                                                                              \
        static CLASS mkVar(                                                                                            \
                smt::context_t& ctx,                                                                                   \
                const std::string& name,                                                                               \
                std::function<Bool(CLASS)> daAxiom) {                                                                  \
            /* first construct the no-axiom version */                                                                 \
            CLASS nav = mkVar(ctx, name);                                                                              \
            return addAxiom(nav, daAxiom(nav));                                                                        \
        }                                                                                                              \
        static CLASS mkFreshVar(smt::context_t& ctx, const std::string& name) {                                        \
            return CLASS{                                                                                              \
                ctx,                                                                                                   \
                smt::mkVar(ctx, getStaticSort(ctx), name, SmtEngine::freshness::fresh)                                 \
            };                                                                                                         \
        }                                                                                                              \
        static CLASS mkFreshVar(                                                                                       \
                smt::context_t& ctx,                                                                                   \
                const std::string& name,                                                                               \
                std::function<Bool(CLASS)> daAxiom) {                                                                  \
            /* first construct the no-axiom version */                                                                 \
            CLASS nav = mkFreshVar(ctx, name);                                                                         \
            return addAxiom(nav, daAxiom(nav));                                                                        \
        }                                                                                                              \
        inline bool isValid() const {                                                                                  \
            return traits::check(getCtx(), getExpr());                                                                 \
        }                                                                                                              \
        inline explicit operator bool() const { return isValid(); }                                                    \

#define ASPECT_END };

#define ASPECT_BEGIN(CLASS) ASPECT_BEGIN_WITH_BASE(CLASS, ValueExpr)

#define ASPECT(CLASS) \
        ASPECT_BEGIN(CLASS) \
        ASPECT_END

////////////////////////////////////////////////////////////////////////////////

class Bool;

namespace impl {
template<>
struct generator<Bool> {
    typedef bool basetype;

    static smt::sort_t sort(smt::context_t& ctx) { return smt::bool_sort(ctx); }
    static bool check(smt::context_t& ctx, smt::expr_t e) { return smt::is_bool(ctx, e); }
    static smt::expr_t mkConst(smt::context_t& ctx, bool b) { return smt::mkBoolConst(ctx, b); }

    static smt::expr_t convert(smt::context_t& ctx, smt::expr_t e) {
        if(smt::is_bool(ctx, e)) return e;
        if(smt::is_bv(ctx, e))
            return smt::binop(ctx, SmtEngine::binOp::NEQUAL, e, smt::mkNumericConst(ctx, smt::get_sort(ctx, e), 0_i64));
        UNREACHABLE("Illegal expression specified: " + smt::toString(e));
    }
};
} // namespace impl

ASPECT_BEGIN(Bool)
    Bool operator!() const {
        auto&& ctx = getCtx();
        return Bool(ctx, smt::unop(ctx, SmtEngine::unOp::NEGATE, getExpr()), getAxiom());
    }
    Bool implies(Bool that) const {
        auto&& ctx = getCtx();
        auto ax = smt_impl::spliceAxioms(*this, that);
        return Bool(ctx, smt::binop(ctx, SmtEngine::binOp::IMPLIES, getExpr(), that.getExpr()), ax);
    }
    Bool iff(Bool that) const {
        auto&& ctx = getCtx();
        auto ax = smt_impl::spliceAxioms(*this, that);
        return Bool(ctx, smt::binop(ctx, SmtEngine::binOp::IFF, getExpr(), that.getExpr()), ax);
    }
    size_t getBitSize() const { return 1; }
ASPECT_END

#define DEFINE_BOOL_BOOL_BINOP(OP, NAME)                                                                               \
    inline Bool operator OP(Bool bv0, Bool bv1) {                                                                      \
        auto&& ctx = bv0.getCtx();                                                                                     \
        auto ax = smt_impl::spliceAxioms(bv0, bv1);                                                                    \
        return Bool(ctx, smt::binop(ctx, SmtEngine::binOp::NAME, bv0.getExpr(), bv1.getExpr()), ax);                   \
    }                                                                                                                  \
    inline Bool operator OP(Bool bv0, bool bv1c) {                                                                     \
        auto&& ctx = bv0.getCtx();                                                                                     \
        auto&& bv1e = smt::mkBoolConst(ctx, bv1c);                                                                     \
        return Bool(ctx, smt::binop(ctx, SmtEngine::binOp::NAME, bv0.getExpr(), bv1e), bv0.getAxiom());                \
    }                                                                                                                  \
    inline Bool operator OP(bool bv0c, Bool bv1) {                                                                     \
        auto&& ctx = bv1.getCtx();                                                                                     \
        auto&& bv0e = smt::mkBoolConst(ctx, bv0c);                                                                     \
        return Bool(ctx, smt::binop(ctx, SmtEngine::binOp::NAME, bv0e, bv1.getExpr()), bv1.getAxiom());                \
    }                                                                                                                  \

DEFINE_BOOL_BOOL_BINOP(==, EQUAL)
DEFINE_BOOL_BOOL_BINOP(!=, NEQUAL)
DEFINE_BOOL_BOOL_BINOP(&&, CONJ)
DEFINE_BOOL_BOOL_BINOP(||, DISJ)
DEFINE_BOOL_BOOL_BINOP(^, XOR)

#undef DEFINE_BOOL_BOOL_BINOP

////////////////////////////////////////////////////////////////////////////////

class AnyBitVector;
template<size_t N> class BitVector;

namespace impl {

template<>
struct generator< AnyBitVector > {
    typedef int64_t basetype;

    static smt::sort_t sort(smt::context_t& ctx) { return smt::bv_sort(ctx, sizeof(basetype) * 8); }
    static bool check(smt::context_t& ctx, smt::expr_t e) { return smt::is_bv(ctx, e); }
    static smt::expr_t mkConst(smt::context_t& ctx, int64_t n) { return smt::mkNumericConst(ctx, sort(ctx), n); }

    static smt::expr_t convert(smt::context_t& ctx, smt::expr_t e) {
        if(smt::is_bool(ctx, e)) {
            auto bv1 = smt::bv_sort(ctx, 1);
            return smt::ite(ctx, e, smt::mkNumericConst(ctx, bv1, 1_i64), smt::mkNumericConst(ctx, bv1, 0_i64));
        }
        ASSERTC(smt::is_bv(ctx, e));
        return e;
    }
};

template<size_t N>
struct generator< BitVector<N> > {
    typedef int64_t basetype;

    static smt::sort_t sort(smt::context_t& ctx) { return smt::bv_sort(ctx, N); }
    static bool check(smt::context_t& ctx, smt::expr_t e) {
        return smt::is_bv(ctx, e) && smt::bv_size(ctx, smt::get_sort(ctx, e)) == N;
    }
    static smt::expr_t mkConst(smt::context_t& ctx, int64_t n) {
        return smt::mkNumericConst(ctx, smt::bv_sort(ctx, N), n);
    }

    static smt::expr_t convert(smt::context_t& ctx, smt::expr_t e) {
        if(smt::is_bool(ctx, e)) {
            return smt::ite(ctx, e, smt::mkNumericConst(ctx, sort(ctx), uint64_t(1)), smt::mkNumericConst(ctx, sort(ctx), uint64_t(0)));
        }
        ASSERTC(smt::is_bv(ctx, e));
        auto size = smt::bv_size(ctx, sort(ctx));
        auto esize = smt::bv_size(ctx, smt::get_sort(ctx, e));
        if(size == esize) return e;
        else if(size > esize) {
            return smt::sext(ctx, e, size - esize);
        } else {
            return smt::extract(ctx, e, size, 0);
        }
        UNREACHABLE("Target bv size less than original");
    }
};
} // namespace impl

ASPECT_BEGIN(AnyBitVector)
public:
    static AnyBitVector mkConst(smt::context_t& ctx, basetype value, size_t bitSize) {
        return AnyBitVector{ ctx, smt::mkNumericConst(ctx, smt::bv_sort(ctx, bitSize), value) };
    }

    static AnyBitVector mkConst(smt::context_t& ctx, const std::string& value, size_t bitSize) {
        return AnyBitVector{ ctx, smt::mkNumericConst(ctx, smt::bv_sort(ctx, bitSize), value) };
    }

    static AnyBitVector mkBound(smt::context_t& ctx, unsigned i, size_t bitSize) {
        return AnyBitVector{ ctx, smt::mkBound(ctx, i, smt::bv_sort(ctx, bitSize)) };
    }

    static AnyBitVector mkVar(smt::context_t& ctx, const std::string& name, size_t bitSize) {
         return AnyBitVector{
             ctx,
             smt::mkVar(ctx, smt::bv_sort(ctx, bitSize), name, SmtEngine::freshness::normal)
         };
    }

    static AnyBitVector mkFreshVar(smt::context_t& ctx, const std::string& name, size_t bitSize) {
        return AnyBitVector{
            ctx,
            smt::mkVar(ctx, smt::bv_sort(ctx, bitSize), name, SmtEngine::freshness::fresh)
        };
    }

    size_t getBitSize() const {
        return smt::bv_size(getCtx(), getSort());
    }

    AnyBitVector sgrowTo(size_t n) const {
        size_t m = getBitSize();
        auto& ctx = getCtx();
        if (m < n)
            return AnyBitVector{
                ctx,
                smt::sext(ctx, getExpr(), n-m),
                getAxiom()
            };
        else return *this;
    }

    AnyBitVector zgrowTo(size_t n) const {
        size_t m = getBitSize();
        auto& ctx = getCtx();
        if (m < n)
            return AnyBitVector{
                ctx,
                smt::zext(ctx, getExpr(), n-m),
                getAxiom()
            };
        else return *this;
    }

    AnyBitVector extract(size_t high, size_t low) const {
        size_t m = getBitSize();
        ASSERT(high < m, "High must be less then bit-vector size.");
        ASSERT(low <= high, "Low mustn't be greater then high.");

        auto& ctx = getCtx();
        return AnyBitVector{
            ctx,
            smt::extract(ctx, getExpr(), high, low),
            getAxiom()
        };
    }

    AnyBitVector adaptTo(size_t N, bool isSigned = false) const {
        if (N > getBitSize()) {
            return isSigned ? sgrowTo(N) : zgrowTo(N);
        } else if (N < getBitSize()) {
            return extract(N - 1, 0);
        } else {
            return *this;
        }
    }

    ValueExpr binary(SmtEngine::binOp op, const AnyBitVector& rhv) const {
        size_t sz = std::max(getBitSize(), rhv.getBitSize());
        AnyBitVector l = sgrowTo(sz);
        AnyBitVector r = rhv.sgrowTo(sz);
        auto& ctx = getCtx();

        auto res = smt::binop(ctx, op, l.getExpr(), r.getExpr());
        auto axm = smt_impl::spliceAxioms(l, r);
        return ValueExpr{ ctx, res, axm };
    }

    template<class Numeric>
    AnyBitVector ofConstant(Numeric n) const {
        auto&& ctx = getCtx();
        return AnyBitVector{ ctx, smt::mkNumericConst(ctx, getSort(), n) };
    }

    AnyBitVector lshr(const AnyBitVector& shift) const {
        return binary(SmtEngine::binOp::LSHR, shift);
    }

    Bool ult(const AnyBitVector& rhv) {
        return binary(SmtEngine::binOp::ULT, rhv);
    }
    template<class Numeric, class = std::enable_if_t<std::is_arithmetic<Numeric>::value>>
    Bool ult(Numeric rhv) {
        return ult(ofConstant(rhv));
    }

    Bool ugt(const AnyBitVector& rhv) {
        return binary(SmtEngine::binOp::UGT, rhv);
    }
    template<class Numeric, class = std::enable_if_t<std::is_arithmetic<Numeric>::value>>
    Bool ugt(Numeric rhv) {
        return ugt(ofConstant(rhv));
    }

    Bool ule(const AnyBitVector& rhv) {
        return binary(SmtEngine::binOp::ULE, rhv);
    }
    template<class Numeric, class = std::enable_if_t<std::is_arithmetic<Numeric>::value>>
    Bool ule(Numeric rhv) {
        return ule(ofConstant(rhv));
    }

    Bool uge(const AnyBitVector& rhv) {
        return binary(SmtEngine::binOp::UGE, rhv);
    }
    template<class Numeric, class = std::enable_if_t<std::is_arithmetic<Numeric>::value>>
    Bool uge(Numeric rhv) {
        return uge(ofConstant(rhv));
    }

ASPECT_END

template<size_t N>
ASPECT_BEGIN_WITH_BASE(BitVector, AnyBitVector)
    enum{ bitsize = N };
    size_t getBitSize() const { return bitsize; }
ASPECT_END

#define REDEF_BV_CMP_OP(OP, IMPLOP)                                                                                    \
        inline Bool operator OP(AnyBitVector bv0, AnyBitVector bv1) {                                                  \
            return bv0.binary(SmtEngine::binOp::IMPLOP, bv1);                                                          \
        }                                                                                                              \
        template<class Numeric, class = std::enable_if_t<std::is_arithmetic<Numeric>::value>>                          \
        Bool operator OP(AnyBitVector bv0, Numeric bv1c) {                                                             \
            return bv0.binary(SmtEngine::binOp::IMPLOP, bv0.ofConstant(bv1c));                                         \
        }                                                                                                              \
        template<class Numeric, class = std::enable_if_t<std::is_arithmetic<Numeric>::value>>                          \
        Bool operator OP(Numeric bv0c, AnyBitVector bv1) {                                                             \
            return bv1.ofConstant(bv0c).binary(SmtEngine::binOp::IMPLOP, bv1);                                         \
        }                                                                                                              \

REDEF_BV_CMP_OP(==, EQUAL)
REDEF_BV_CMP_OP(!=, NEQUAL)
REDEF_BV_CMP_OP(>, SGT)
REDEF_BV_CMP_OP(>=, SGE)
REDEF_BV_CMP_OP(<=, SLE)
REDEF_BV_CMP_OP(<, SLT)

#undef REDEF_BV_CMP_OP

#define REDEF_BV_BIN_OP(OP, IMPLOP)                                                                                    \
        inline AnyBitVector operator OP(AnyBitVector bv0, AnyBitVector bv1) {                                          \
            return bv0.binary(SmtEngine::binOp::IMPLOP, bv1);                                                          \
        }                                                                                                              \
        template<class Numeric, class = std::enable_if_t<std::is_arithmetic<Numeric>::value>>                          \
        AnyBitVector operator OP(AnyBitVector bv0, Numeric bv1c) {                                                     \
            return bv0.binary(SmtEngine::binOp::IMPLOP, bv0.ofConstant(bv1c));                                         \
        }                                                                                                              \
        template<class Numeric, class = std::enable_if_t<std::is_arithmetic<Numeric>::value>>                          \
        AnyBitVector operator OP(Numeric bv0c, AnyBitVector bv1) {                                                     \
            return bv1.ofConstant(bv0c).binary(SmtEngine::binOp::IMPLOP, bv1);                                         \
        }                                                                                                              \

REDEF_BV_BIN_OP(+, ADD)
REDEF_BV_BIN_OP(-, SUB)
REDEF_BV_BIN_OP(*, SMUL)
REDEF_BV_BIN_OP(/, SDIV)
REDEF_BV_BIN_OP(%, SREM)
REDEF_BV_BIN_OP(|, BOR)
REDEF_BV_BIN_OP(&, BAND)
REDEF_BV_BIN_OP(^, BXOR)
REDEF_BV_BIN_OP(<<, ASHL)
REDEF_BV_BIN_OP(>>, LSHR)

#undef REDEF_BV_BIN_OP

#define REDEF_UN_OP(OP, IMPLOP)                                                                                        \
        inline AnyBitVector operator OP(AnyBitVector bv) {                                                             \
            auto&& ctx = bv.getCtx();                                                                                  \
            return AnyBitVector{                                                                                       \
                ctx,                                                                                                   \
                smt::unop(ctx, SmtEngine::unOp::IMPLOP, bv.getExpr()),                                                 \
                bv.getAxiom()                                                                                          \
            };                                                                                                         \
        }                                                                                                              \

REDEF_UN_OP(~, NEGATE)
REDEF_UN_OP(-, UMINUS)

#undef REDEF_UN_OP

////////////////////////////////////////////////////////////////////////////////

inline Bool operator==(const ValueExpr& l, const ValueExpr& r) {
    Bool lb = l;
    Bool rb = r;
    if(bool(lb) && bool(rb)) return lb == rb;
    AnyBitVector lbv = l;
    AnyBitVector rbv = r;
    if(bool(lbv) && bool(rbv) && lbv.getBitSize() == rbv.getBitSize()) return lbv == rbv;
    UNREACHABLE("(==): invalid operands")
}

inline Bool operator!=(const ValueExpr& l, const ValueExpr& r) {
    Bool lb = l;
    Bool rb = r;
    if(bool(lb) && bool(rb)) return lb != rb;
    AnyBitVector lbv = l;
    AnyBitVector rbv = r;
    if(bool(lbv) && bool(rbv) && lbv.getBitSize() == rbv.getBitSize()) return lbv != rbv;
    UNREACHABLE("(!=): invalid operands")
}

namespace impl_ {
template<class T0, class T1, class SFINAE = void>
struct merger;
template<class X>
struct merger<X, X> {
    using type = X;
};
template<size_t N>
struct merger<AnyBitVector, BitVector<N>>
{
    using type = AnyBitVector;
};
template<size_t N>
struct merger<BitVector<N>, AnyBitVector>
{
    using type = AnyBitVector;
};
template<size_t N1, size_t N2>
struct merger<BitVector<N1>, BitVector<N2>, std::enable_if_t<N1 != N2>>
{
    using type = BitVector< (N1 > N2)? N1 : N2 >;
};

}

template<class L, class R>
using merged_t = typename impl_::merger<L, R>::type;

inline const Bool& mergeTo(const Bool& l, Bool) { return l; }
inline AnyBitVector mergeTo(const AnyBitVector& l, const AnyBitVector& r) {
    auto sz = std::max(l.getBitSize(), r.getBitSize());
    return l.sgrowTo(sz);
}
template<size_t N>
const BitVector<N>& mergeTo(const BitVector<N>& l, const BitVector<N>&) {
    return l;
}
inline const ValueExpr& mergeTo(const ValueExpr& l, const ValueExpr&) {
    return l;
}

struct ifer {
    template<class E>
    struct elser {
        Bool cond;
        E tbranch;

        template<class E2>
        inline merged_t<E,E2> else_(E2 fbranch) {
            auto& ctx = cond.getCtx();
            auto&& tb = mergeTo(tbranch, fbranch);
            auto&& fb = mergeTo(fbranch, tbranch);

            return merged_t<E, E2>{
                ctx,
                smt::ite(
                    ctx,
                    cond.getExpr(),
                    tb.getExpr(),
                    fb.getExpr()
                ),
                smt_impl::spliceAxioms(cond, tb, fb)
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

namespace smt_impl {

template<class ExprCollection, class F, class Types, size_t ...Ixs>
auto applyLimited2(util::indexer<Ixs...>, Types, smt::context_t& ctx, const ExprCollection& vec, F f) {
    return f( util::index_in_type_list_q<Ixs, Types>{ ctx, vec[Ixs] } ... );
}

template<class F, class ...ExprTypes>
auto applyLimited(smt::context_t& ctx, const std::vector<smt::expr_t>& vec, F f) {
    return applyLimited2(
        typename util::make_indexer<ExprTypes...>::type{},
        util::type_list<ExprTypes...>{},
        ctx,
        vec,
        f
    );
}

} // namespace smt_impl

////////////////////////////////////////////////////////////////////////////////

namespace smt_impl {

template<class Expr>
smt::pattern_t make_pattern(smt::context_t &ctx, Expr e) {
    return smt::mkPattern(ctx, e.getExpr());
}

} /* smt_impl */

template<class ...Args>
Bool forAll(
    smt::context_t& ctx,
    std::function<Bool(Args...)> func
) {

    std::vector<smt::sort_t> sorts { impl::generator<Args>::sort(ctx)... };

    std::vector<smt::expr_t> axioms;

    auto eret = smt::mkForAll(
        ctx,
        sorts,
        [&](const std::vector<smt::expr_t>& args) {
            auto&& ret = smt_impl::applyLimited<decltype(func), Args...>(ctx, args, func);
            axioms.push_back(ret.getAxiom()); // this is a bit weak
            return ret.getExpr();
        }
    );

    return Bool(ctx, eret, smt_impl::spliceAxiomsImpl(ctx, axioms));
}

template<class VarT>
Bool forAllConst(smt::context_t& ctx, VarT var, Bool body) {
    return Bool(ctx, smt::mkForAll(ctx, var.getExpr(), body.getExpr()), smt_impl::spliceAxioms(var, body) );
}

template<class Res, class Patterns, class ...Args>
Bool forAll(
    smt::context_t& ctx,
    std::function<Res(Args...)> func,
    std::function<Patterns(Args...)> patternGenerator
) {

    std::vector<smt::sort_t> sorts { impl::generator<Args>::sort(ctx)... };

    std::vector<smt::expr_t> axioms;

    auto eret = smt::mkForAll(
        ctx,
        sorts,
        [&](const std::vector<smt::expr_t>& args) -> smt::expr_t {
            auto&& ret = smt_impl::applyLimited<decltype(func), Args...>(ctx, args, func);
            axioms.push_back(ret.getAxiom()); // this is a bit weak
            return ret.getExpr();
        },
        [&](const std::vector<smt::expr_t>& args) -> std::vector<smt::pattern_t> {
            auto&& ret = smt_impl::applyLimited<decltype(patternGenerator), Args...>(ctx, args, patternGenerator);
            return util::viewContainer(ret).map(LAM(p, smt::mkPattern(ctx, p.getExpr()))).toVector();
        }
    );

    return Bool(ctx, eret, smt_impl::spliceAxiomsImpl(ctx, axioms));
}

////////////////////////////////////////////////////////////////////////////////

struct add_no_overflow {
    static AnyBitVector doit(const AnyBitVector& bv0, const AnyBitVector& bv1, bool isSigned) {
        auto& ctx = bv0.getCtx();

        auto sz = std::max(bv0.getBitSize(), bv1.getBitSize());
        auto ebv0 = bv0.adaptTo(sz, isSigned);
        auto ebv1 = bv1.adaptTo(sz, isSigned);

        auto zero = ebv0.ofConstant(0_i64);

        auto zero_ = BitVector<1>::mkConst(ctx, 0_i64);

        if (isSigned) {
            auto res = ebv0 + ebv1;
            auto axm = (ebv0 > zero && ebv1 > zero).implies(res > zero) &&
                       (ebv0 < zero && ebv1 < zero).implies(res < zero);
            return res.withAxiom(axm);
        } else {

            ebv0 = ebv0.zgrowTo(sz + 1);
            ebv1 = ebv1.zgrowTo(sz + 1);

            auto res = ebv0 + ebv1;
            auto axm = zero_ == res.extract(sz, sz);

            return res.extract(sz - 1, 0).withAxiom(axm);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem>
Bool distinct(smt::context_t& ctx, const std::vector<Elem>& elems) {
    if (elems.empty()) return Bool::mkConst(ctx, true);

    std::vector<smt::expr_t> cast;
    for (const auto& elem : elems) {
        cast.push_back(elem.getExpr());
    }

    smt::expr_t ret = smt::distinct(ctx, cast);

    auto axiom = smt_impl::defaultAxiom(ctx);
    for (const auto& elem : elems) {
        axiom = smt_impl::spliceAxiomsImpl(ctx, axiom, elem.getAxiom());
    }

    return Bool{ ctx, ret, axiom };
}

////////////////////////////////////////////////////////////////////////////////

template< class >
class Function; // undefined

template<class Res, class ...Args>
class Function<Res(Args...)> : public Expr {
    smt::function_t inner;
    smt::expr_t axiomatic;
    smt::context_t* ctx;

    template<class ...CArgs>
    inline static smt::expr_t massAxiomAnd(CArgs... args) {
        static_assert(sizeof...(args) > 0, "Trying to massAxiomAnd zero axioms");
        return smt_impl::spliceAxioms(args...);
    }

    static smt::function_t constructFunc(smt::context_t& ctx, const std::string& name) {
        std::vector<smt::sort_t> domain { impl::generator<Args>::sort(ctx)... };

        return smt::mkFunction(ctx, name, impl::generator<Res>::sort(ctx), domain);
    }

    static smt::function_t constructFreshFunc(smt::context_t& ctx, const std::string& name) {
        std::vector<smt::sort_t> domain { impl::generator<Args>::sort(ctx)... };

        return smt::mkFreshFunction(ctx, name, impl::generator<Res>::sort(ctx), domain);
    }

    static smt::expr_t constructAxiomatic(
        smt::context_t& ctx,
        smt::function_t smtfunc,
        std::function<Res(Args...)> realfunc) {

        std::function<Bool(Args...)> lam = [&ctx, smtfunc, realfunc](Args... args) -> Bool {
            std::vector<smt::expr_t> args_{ args.getExpr()... };
            auto ourRes = realfunc(args...);
            auto theirRes = smt::applyFunction(ctx, smtfunc, args_);
            return Bool(ctx, smt::binop(ctx, SmtEngine::binOp::EQUAL, ourRes.getExpr(), theirRes), ourRes.getAxiom());
        };

        return forAll(ctx, lam).getExpr();
    }

public:

    typedef Function Self;

    Function(const Function&) = default;
    Function(smt::context_t& ctx, smt::function_t inner, smt::expr_t axiomatic):
        inner(inner), axiomatic(axiomatic), ctx(&ctx) {};
    explicit Function(smt::context_t& ctx, smt::function_t inner):
        inner(inner), axiomatic(smt_impl::defaultAxiom(ctx)), ctx(&ctx) {};
    Function(smt::context_t& ctx, const std::string& name, std::function<Res(Args...)> f):
        inner(constructFunc(ctx, name)), axiomatic(constructAxiomatic(ctx, inner, f)), ctx(&ctx){}

    smt::function_t getFunction() const { return inner; }
    smt::expr_t getAxiom() const { return axiomatic; }
    smt::context_t& getCtx() const { return *ctx; }

    Self withAxiom(const ValueExpr& ax) const {
        return Self{ inner, smt_impl::spliceAxiomsImpl(getAxiom(), ax.asAxiom()) };
    }

    Res operator()(Args... args) const {
        auto&& ctx = getCtx();
        std::vector<smt::expr_t> smtArgs{ args.getExpr()... };
        return Res(ctx, smt::apply(ctx, inner, smtArgs), smt_impl::spliceAxiomsImpl(ctx, getAxiom(), massAxiomAnd(args...)));
    }

    static smt::sort_t range(smt::context_t& ctx) {
        return impl::generator<Res>::sort(ctx);
    }

    template<size_t N = 0>
    static smt::sort_t domain(smt::context_t& ctx) {
        return impl::generator< util::index_in_row_q<N, Args...> >::sort(ctx);
    }

    static Self mkFunc(smt::context_t& ctx, const std::string& name) {
        return Self{ constructFunc(ctx, name) };
    }

    static Self mkFunc(smt::context_t& ctx, const std::string& name, std::function<Res(Args...)> body) {
        smt::function_t f = constructFunc(ctx, name);
        smt::expr_t ax = constructAxiomatic(ctx, f, body);
        return Self{ ctx, f, ax };
    }

    static Self mkFreshFunc(smt::context_t& ctx, const std::string& name) {
        return Self{ ctx, constructFreshFunc(ctx, name) };
    }

    static Self mkFreshFunc(smt::context_t& ctx, const std::string& name, std::function<Res(Args...)> body) {
        smt::function_t f = constructFreshFunc(ctx, name);
        smt::expr_t ax = constructAxiomatic(ctx, f, body);
        return Self{ ctx, f, ax };
    }

    static Self mkDerivedFunc(
        smt::context_t& ctx,
        const std::string& name,
        const Self& oldFunc,
        std::function<Res(Args...)> body) {
        smt::function_t f = constructFreshFunc(ctx, name);
        smt::expr_t ax = constructAxiomatic(ctx, f, body);
        return Self{ ctx, f, smt_impl::spliceAxiomsImpl(oldFunc.getAxiom(), ax) };
    }

    static Self mkDerivedFunc(
        smt::context_t& ctx,
        const std::string& name,
        const std::vector<Self>& oldFuncs,
        std::function<Res(Args...)> body) {
        smt::function_t f = constructFreshFunc(ctx, name);

        std::vector<smt::expr_t> axs;
        axs.reserve(oldFuncs.size() + 1);
        std::transform(oldFuncs.begin(), oldFuncs.end(), std::back_inserter(axs),
                       [](const Self& oldFunc) { return oldFunc.getAxiom(); });
        axs.push_back(constructAxiomatic(ctx, f, body));

        return Self{ ctx, f, smt_impl::spliceAxiomsImpl(ctx, axs) };
    }
};

template<class Res, class ...Args>
std::ostream& operator<<(std::ostream& ost, Function<Res(Args...)> f) {
    return ost << f.getFunction() << " assuming " << f.getAxiom();
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
    FuncArray(smt::context_t& ctx, const std::string& name):
        FuncArray(name, inner_t::mkFreshFunc(ctx, name)) {}
    FuncArray(smt::context_t& ctx, const std::string& name, std::function<Elem(Index)> f):
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

    smt::context_t& ctx() const { return inner.ctx(); }

    static Self mkDefault(smt::context_t& ctx, const std::string& name, Elem def) {
        return Self{ ctx, name, [def](Index){ return def; } };
    }

    static Self mkFree(smt::context_t& ctx, const std::string& name) {
        return Self{ ctx, name };
    }

    static Self mkVar(smt::context_t& ctx, const std::string& name) {
        return Self{ name, inner_t::mkFunc(ctx, name) };
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

    Bool operator==(Self) const {
        BYE_BYE(Bool, "operator== not supported by function-based array");
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class TheoryArray: public ValueExpr {

public:

    TheoryArray(smt::context_t& ctx, smt::expr_t inner) : ValueExpr(ctx, inner) {
        ASSERT(smt::is_array(ctx, inner), "TheoryArray constructed from non-array");
    };
    TheoryArray(smt::context_t& ctx, smt::expr_t inner, smt::expr_t axioms) : ValueExpr(ctx, inner, axioms) {
        ASSERT(smt::is_array(ctx, inner), "TheoryArray constructed from non-array");
    };
    TheoryArray(const TheoryArray&) = default;

    Elem select (Index i) const {
        auto&& ctx = getCtx();
        return Elem(
            ctx,
            smt::binop(ctx, SmtEngine::binOp::LOAD, getExpr(), i.getExpr())
        );
    }
    Elem operator[](Index i) const { return select(i); }

    TheoryArray store(Index i, Elem e) {
        auto&& ctx = getCtx();
        return TheoryArray(ctx, smt::store(ctx, getExpr(), i.getExpr(), e.getExpr()));
    }

    TheoryArray store(const std::vector<std::pair<Index, Elem>>& entries) {
        smt::expr_t base = getExpr();
        auto&& ctx = getCtx();
        for (const auto& entry : entries) {
            base = smt::store(ctx, base, entry.first.getExpr(), entry.second.getExpr());
        }
        return TheoryArray(ctx, base);
    }

    static TheoryArray mkDefault(smt::context_t& ctx, const std::string&, Elem def) {
        return TheoryArray( ctx, smt::mkArrayConst(ctx, impl::generator<Index>::sort(ctx), def.getExpr()));
    }

    static TheoryArray mkFree(smt::context_t& ctx, const std::string& name) {
        auto as = smt::array_sort(
            ctx,
            impl::generator<Index>::sort(ctx),
            impl::generator<Elem>::sort(ctx)
        );
        return TheoryArray(ctx, smt::mkVar(ctx, as, name, SmtEngine::freshness::fresh));
    }

    static TheoryArray mkVar(smt::context_t& ctx, const std::string& name) {
        auto as = smt::array_sort(
            ctx,
            impl::generator<Index>::sort(ctx),
            impl::generator<Elem>::sort(ctx)
        );
        return TheoryArray(ctx, smt::mkVar(ctx, as, name, SmtEngine::freshness::normal));
    }

    static TheoryArray merge(
        const std::string&,
        TheoryArray defaultArray,
        const std::vector<std::pair<Bool, TheoryArray>>& arrays) {
        return switch_(arrays, defaultArray);
    }

    Bool operator==(TheoryArray that) const {
        auto&& ctx = getCtx();
        auto ax = smt_impl::spliceAxioms(*this, that);
        return Bool(ctx, smt::binop(ctx, SmtEngine::binOp::EQUAL, getExpr(), that.getExpr()), ax);
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Elem, class Index>
class InlinedFuncArray {
    typedef InlinedFuncArray<Elem, Index> Self;
    typedef std::function<Elem(Index)> inner_t;

    smt::context_t* context;
    std::shared_ptr<std::string> name;
    inner_t inner;

    InlinedFuncArray(
        smt::context_t& ctx,
        std::shared_ptr<std::string> name,
        inner_t inner
    ) : context(&ctx), name(name), inner(inner) {};

public:

    InlinedFuncArray(const InlinedFuncArray&) = default;
    InlinedFuncArray(smt::context_t& ctx, const std::string& name, std::function<Elem(Index)> f):
        context(&ctx), name(std::make_shared<std::string>(name)), inner(f) {}
    InlinedFuncArray(smt::context_t& ctx, const std::string& name):
        context(&ctx), name(std::make_shared<std::string>(name)) {

        // XXX akhin this is as fucked up as before, but also works for now

        auto initial = Function<Elem(Index)>::mkFreshFunc(ctx, "(initial)" + name);
        inner = [initial](Index ix) -> Elem {
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

    smt::context_t& getCtx() const { return *context; }

    static Self mkDefault(smt::context_t& ctx, const std::string& name, Elem def) {
        return Self{ ctx, name, [def](Index){ return def; } };
    }

    static Self mkFree(smt::context_t& ctx, const std::string& name) {
        return Self{ ctx, name };
    }

    static Self mkVar(smt::context_t& /* ctx */, const std::string& /* name */) {
        BYE_BYE(Self, "mkVar not supported");
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

        return Self{ defaultArray.getCtx(), name, nf };
    }

    Bool operator==(Self) const {
        BYE_BYE(Bool, "operator== not supported by inlined function arrays");
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
        return std::vector<Byte>{ bv.sgrowTo(ElemSize) };
    }

    std::vector<Byte> ret;
    for (size_t ix = 0; ix < N; ix += ElemSize) {
        auto hi = std::min(ix + ElemSize - 1, N - 1);
        ret.push_back( Byte::forceCast(bv.extract(hi, ix)) );
    }
    return ret;
}

template<size_t ElemSize = 8>
std::vector<BitVector<ElemSize>> splitBytes(const ValueExpr& bv) {
    typedef BitVector<ElemSize> Byte;

    AnyBitVector dbv = bv;
    ASSERT(dbv, "Non-vector");

    size_t width = dbv.getBitSize();

    if (width <= ElemSize) {
        Byte byte = dbv.sgrowTo(ElemSize);
        ASSERT(byte, "Invalid dynamic BitVector, cannot convert to Byte");
        return std::vector<Byte>{ byte };
    }

    std::vector<Byte> ret;
    for (size_t ix = 0; ix < width; ix += ElemSize) {
        auto hi = std::min(ix + ElemSize - 1, width - 1);
        ret.push_back( Byte::forceCast(dbv.extract(hi, ix)) );
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

    smt::expr_t head = bytes[0].getExpr();
    smt::context_t& ctx = bytes[0].getCtx();

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = smt::binop(ctx, SmtEngine::binOp::CONCAT, bytes[i].getExpr(), head);
    }

    smt::expr_t axiom = bytes[0].getAxiom();
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = smt_impl::spliceAxiomsImpl(ctx, bytes[i].getAxiom(), axiom);
    }

    return BitVector<N>{ ctx, head, axiom };
}

template<size_t ElemSize = 8>
AnyBitVector concatBytesDynamic(const std::vector<BitVector<ElemSize>>& bytes, size_t bitSize) {
    typedef BitVector<ElemSize> Byte;

    using borealis::util::toString;

    smt::expr_t head = bytes[0].getExpr();
    smt::context_t& ctx = bytes[0].getCtx();

    for (size_t i = 1; i < bytes.size(); ++i) {
        head = smt::binop(ctx, SmtEngine::binOp::CONCAT, bytes[i].getExpr(), head);
    }

    smt::expr_t axiom = bytes[0].getAxiom();
    for (size_t i = 1; i < bytes.size(); ++i) {
        axiom = smt_impl::spliceAxiomsImpl(ctx, bytes[i].getAxiom(), axiom);
    }

    return AnyBitVector{ ctx, head, axiom }.adaptTo(bitSize);
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

    AnyBitVector select(Index i, size_t elemBitSize) const {
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

    smt::context_t& getCtx() const { return inner.getCtx(); }

    Byte operator[](Index i) const {
        return inner[i];
    }

    Byte operator[](long long i) const {
        return inner[Index::mkConst(getCtx(), i)];
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

    ScatterArray store(Index i, const ValueExpr& e) {
        AnyBitVector bv = e;
        ASSERTC(bv);
        auto bitsize = bv.getBitSize();
        return store(i, e, bitsize);
    }

    static ScatterArray mkDefault(smt::context_t& ctx, const std::string& name, Byte def) {
        return ScatterArray{ Inner::mkDefault(ctx, name, def) };
    }

    static ScatterArray mkFree(smt::context_t& ctx, const std::string& name) {
        return ScatterArray{ Inner::mkFree(ctx, name) };
    }

    static ScatterArray mkVar(smt::context_t& ctx, const std::string& name) {
        return ScatterArray{ Inner::mkVar(ctx, name) };
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

    Bool operator==(ScatterArray other) const {
        return inner == other.inner;
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

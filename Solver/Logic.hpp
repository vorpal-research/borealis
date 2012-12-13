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

class LogicExpr {};

struct ConversionException: public std::exception {
    std::string message;

    ConversionException(const std::string& mes): std::exception(), message(mes) {}

    virtual const char* what() const throw() {
        return message.c_str();
    }
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

}// namespace z3impl

class LogicExprExpr: public LogicExpr {
    z3::expr inner;
    z3::expr axiomatic;
public:
    LogicExprExpr(const LogicExprExpr&) = default;
    LogicExprExpr(z3::expr e, z3::expr axiom):
        inner(e), axiomatic(axiom){}
    LogicExprExpr(z3::expr e):
        inner(e), axiomatic(z3impl::defaultAxiom(e)){}

    z3::expr get() {
        return inner;
    }

    z3::expr axiom() {
        return axiomatic;
    }
};

template<class Expr0, class Expr1>
inline z3::expr spliceAxioms(Expr0 e0, Expr1 e1) {
    return z3impl::spliceAxioms(e0.axiom(), e1.axiom());
}


class Bool: public LogicExprExpr {
public:
    typedef Bool self;

    Bool(const Bool&) = default;
    Bool(z3::expr e, z3::expr axiom): LogicExprExpr(e, axiom){
        if(!(e.is_bool())) {
            throw ConversionException(
                    "Trying to construct bool from not-bool: " +
                    borealis::util::toString(e)
            );
        }
    }
    Bool(z3::expr e): LogicExprExpr(e) {
        if(!(e.is_bool())) {
            throw ConversionException(
                    "Trying to construct bool from not-bool: " +
                    borealis::util::toString(e)
            );
        }
    }

private:
    Bool(z3::context& ctx, Z3_ast ast): Bool(z3::to_expr(ctx, ast)){};
public:

    Bool implies(Bool that) {
        z3::expr lhv = get();
        z3::expr rhv = that.get();
        return self(to_expr(lhv.ctx(), Z3_mk_implies(lhv.ctx(), lhv, rhv)), spliceAxioms(*this, that));
    }

    Bool iff(Bool that) {
        z3::expr lhv = get();
        z3::expr rhv = that.get();
        return self(to_expr(lhv.ctx(), Z3_mk_iff(lhv.ctx(), lhv, rhv)), spliceAxioms(*this, that));
    }

    z3::expr toAxiom() {
        return get() && axiom();
    }

    static z3::sort sort(z3::context& ctx) {
        return ctx.bool_sort();
    }

    static self mkBound(z3::context& ctx, unsigned i) {
        return z3::to_expr(ctx, Z3_mk_bound(ctx, i, sort(ctx)));
    }

    static self mkConst(z3::context& ctx, bool value) {
        return ctx.bool_val(value);
    }

    static self mkVar(z3::context& ctx, const std::string& name) {
        return ctx.bool_const(name.c_str());
    }

    static self mkVar(
            z3::context& ctx,
            const std::string& name,
            std::function<Bool(Bool)> daAxiom) {
        // first construct the no-axiom version
        self cnst = mkVar(ctx, name);
        return self(cnst.get(), z3impl::spliceAxioms(cnst.axiom(), daAxiom(cnst).toAxiom()));
    }

    static self mkFreshVar(z3::context& ctx, const std::string& name) {
        return self(ctx, Z3_mk_fresh_const(ctx, name.c_str(), sort(ctx)));
    }

    static self mkFreshVar(
            z3::context& ctx,
            const std::string& name,
            std::function<Bool(Bool)> daAxiom) {
        // first construct the no-axiom version
        self cnst = mkFreshVar(ctx, name);
        return self(cnst.get(), z3impl::spliceAxioms(cnst.axiom(), daAxiom(cnst).toAxiom()));
    }

};

std::ostream& operator << (std::ostream& ost, Bool b) {
    return ost << b.get().simplify() << " assuming " << b.axiom().simplify();
}

#define REDEF_BOOL_BOOL_OP(OP) \
        Bool operator OP(Bool bv0, Bool bv1) { \
            return Bool(bv0.get() OP bv1.get(), spliceAxioms(bv0, bv1)); \
        }

REDEF_BOOL_BOOL_OP(==)
REDEF_BOOL_BOOL_OP(!=)
REDEF_BOOL_BOOL_OP(&&)
REDEF_BOOL_BOOL_OP(||)

#undef REDEF_BOOL_BOOL_OP

Bool operator !(Bool bv0) {
    return Bool(!bv0.get(), bv0.axiom());
}


template<size_t N>
class BitVector: public LogicExprExpr {


public:
    typedef BitVector self;

    BitVector(const self&) = default;
    BitVector(z3::expr e, z3::expr axiom): LogicExprExpr(e, axiom){
        if(!(e.is_bv() && e.get_sort().bv_size() == N)) {
            throw ConversionException(
                    "Trying to construct bitvector<" +
                    borealis::util::toString(N) +
                    "> from: " +
                    borealis::util::toString(e));
        }
    }
    explicit BitVector(z3::expr e): LogicExprExpr(e){
        if(!(e.is_bv() && e.get_sort().bv_size() == N)) {
            throw ConversionException(
                    "Trying to construct bitvector<" +
                    borealis::util::toString(N) +
                    "> from: " +
                    borealis::util::toString(e));
        }
    }
private:
    BitVector(z3::context& ctx, Z3_ast ast): BitVector(z3::to_expr(ctx, ast)){}
public:
    static z3::sort sort(z3::context& ctx) {
        return ctx.bv_sort(N);
    }

    static self mkBound(z3::context& ctx, unsigned i) {
        return self(z3::to_expr(ctx, Z3_mk_bound(ctx, i, sort(ctx))));
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
        self cnst = mkVar(ctx, name);
        return self(cnst.get(), z3impl::spliceAxioms(cnst.axiom(), daAxiom(cnst).toAxiom()));
    }

    static self mkFreshVar(z3::context& ctx, const std::string& name) {
        return self(ctx, Z3_mk_fresh_const(ctx, name.c_str(), sort(ctx)));
    }

    static self mkFreshVar(
            z3::context& ctx,
            const std::string& name,
            std::function<Bool(Bool)> daAxiom) {
        // first construct the no-axiom version
        self cnst = mkFreshVar(ctx, name);
        return self(cnst.get(), z3impl::spliceAxioms(cnst.axiom(), daAxiom(cnst).toAxiom()));
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

constexpr size_t max(size_t N0, size_t N1) {
    return N0 > N1 ? N0 : N1;
}

template<class T0, class T1>
struct merger;

template<size_t N0, size_t N1>
struct merger<BitVector<N0>, BitVector<N1>> {
    enum{ M = max(N0,N1) };
    typedef BitVector<M> type;

    static type app(BitVector<N0> bv0) {
        return grow<M>(bv0);
    }

    static type app(BitVector<N1> bv1) {
        return grow<M>(bv1);
    }
};


#define REDEF_BV_BOOL_OP(OP) \
        template<size_t N0, size_t N1, size_t M = max(N0, N1)> \
        Bool operator OP(BitVector<N0> bv0, BitVector<N1> bv1) { \
            auto ebv0 = grow<M>(bv0); \
            auto ebv1 = grow<M>(bv1); \
            return Bool(ebv0.get() OP ebv1.get(), spliceAxioms(ebv0, ebv1)); \
        } \


REDEF_BV_BOOL_OP(==)
REDEF_BV_BOOL_OP(!=)
REDEF_BV_BOOL_OP(>)
REDEF_BV_BOOL_OP(>=)
REDEF_BV_BOOL_OP(<=)
REDEF_BV_BOOL_OP(<)

#undef REDEF_BV_BOOL_OP

#define REDEF_BV_BIN_OP(OP) \
        template<size_t N0, size_t N1, size_t M = max(N0, N1)> \
        BitVector<M> operator OP(BitVector<N0> bv0, BitVector<N1> bv1) { \
            auto ebv0 = grow<M>(bv0); \
            auto ebv1 = grow<M>(bv1); \
            return BitVector<M>(ebv0.get() OP ebv1.get(), spliceAxioms(ebv0, ebv1)); \
        } \


REDEF_BV_BIN_OP(+)
REDEF_BV_BIN_OP(-)
REDEF_BV_BIN_OP(*)
REDEF_BV_BIN_OP(/)

#undef REDEF_BV_BIN_OP

template<size_t N>
std::ostream& operator << (std::ostream& ost, BitVector<N> bv) {
    return ost << bv.get().simplify() << " assuming " << bv.axiom().simplify();
}


struct {
    template<class E>
    struct elser {
        Bool cond;
        E tbranch;

        template<class E2>
        typename merger<E,E2>::type else_(E2 fbranch) {
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

    struct thenr {
        Bool cond;

        template<class E>
        elser<E> then_(E tbranch) {
            return elser<E> { cond, tbranch };
        }
    };

    thenr operator()(Bool cond) {
        return thenr {cond};
    }
} if_;



namespace impl {

template<class Tl, size_t ...N>
std::tuple<typename util::index_in_type_list<N, Tl>::type...>
mkbounds_step_1(z3::context& ctx, Tl, util::indexer<N...>) {
    using namespace borealis::util;

    return std::tuple<typename util::index_in_type_list<N, Tl>::type...>{
        util::index_in_type_list<N, Tl>::type::mkBound(ctx, N)...
    };
}

} // namespace _impl

template<class ...Args>
std::tuple<Args...> mkBounds(z3::context& ctx) {
    using namespace borealis::util;
    return impl::mkbounds_step_1(ctx, type_list<Args...>(), typename make_indexer<Args...>::type());
}

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
        std::string name = "forall_bound_" + toString(numBounds - i -1);
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

} // namespace borealis::logic::z3impl

template<class ...Args>
Bool forAll(
        z3::context& ctx,
        std::function<Bool(Args...)> func
    ) {
    return Bool(z3impl::forAll(ctx, func));
}

template< class >
class Function; // undefined

template<class Res, class ...Args>
class Function<Res(Args...)> : public LogicExpr {
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

    z3::func_decl get() { return inner; }
    z3::expr axiom() { return axiomatic; }

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
std::ostream& operator << (std::ostream& ost, Function<Res(Args...)> f) {
    return ost << f.get() << " assuming " << f.axiom().simplify();
}

template<class Elem, class Index>
class FuncArray {
    std::shared_ptr<std::string> name;
    typedef Function<Elem(Index)> inner_t;

    inner_t inner;

    FuncArray(inner_t inner, std::shared_ptr<std::string>& name): name(name), inner(inner){};
public:
    FuncArray(const FuncArray&) = default;
    FuncArray(z3::context& ctx, const std::string& name, std::function<Elem(Index)> f):
        name(std::make_shared<std::string>(name)), inner(inner_t::mkFreshFunc(ctx, name, f)){}


    Elem select    (Index i) { return inner(i);  }
    Elem operator[](Index i) { return select(i); }

    FuncArray<Elem, Index> store(Index i, Elem e) {
        inner_t nf = inner_t::mkFreshFunc(inner.get().ctx(), *name, [this](Index res){
            return if_(res == i).then_(e).else_(this->select(res));
        });

        return FuncArray<Elem, Index> (nf, name);
    }
};

}
}

#endif // LOGIC_HPP

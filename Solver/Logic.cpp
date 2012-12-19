/*
 * Logic.cpp
 *
 *  Created on: Dec 12, 2012
 *      Author: belyaev
 */

#include "Logic.hpp"


namespace borealis {
namespace logic{

ConversionException::ConversionException(const std::string& mes):
        std::exception(), message(mes) {}

const char* ConversionException::what() const throw() {
    return message.c_str();
}

struct LogicExprExpr::Impl {
    z3::expr inner;
    z3::expr axiomatic;
};

LogicExprExpr::LogicExprExpr(const LogicExprExpr& that):
    pimpl(new Impl(*that.pimpl)) {}

LogicExprExpr::LogicExprExpr(LogicExprExpr&& that):
    pimpl((that.pimpl)) {}

LogicExprExpr::LogicExprExpr(z3::expr e, z3::expr axiom):
    pimpl(new Impl{e, axiom}) {}

LogicExprExpr::LogicExprExpr(z3::expr e):
    pimpl(new Impl{e, z3impl::defaultAxiom(e)}) {}

LogicExprExpr::~LogicExprExpr() {
    delete pimpl;
}

void LogicExprExpr::swap(LogicExprExpr& that) {
    std::swap(pimpl, that.pimpl);
}

LogicExprExpr& LogicExprExpr::operator=(const LogicExprExpr& that) {
    LogicExprExpr e(that);
    swap(e);
    return *this;
}

z3::expr LogicExprExpr::get() const {
    return pimpl->inner;
}

z3::expr LogicExprExpr::axiom() const {
    return pimpl->axiomatic;
}

z3::sort LogicExprExpr::get_sort() const {
    return pimpl->inner.get_sort();
}

Bool::Bool(const Bool&) = default;

Bool::Bool(z3::expr e, z3::expr axiom) :
        LogicExprExpr(e, axiom) {
    if (!(e.is_bool())) {
        throw ConversionException(
                "Trying to construct bool from not-bool: "
                        + borealis::util::toString(e));
    }
}

Bool::Bool(z3::expr e) :
        LogicExprExpr(e) {
    if (!(e.is_bool())) {
        throw ConversionException(
                "Trying to construct bool from not-bool: "
                        + borealis::util::toString(e));
    }
}

Bool::Bool(z3::context& ctx, Z3_ast ast): Bool(z3::to_expr(ctx, ast)){};

Bool Bool::implies(Bool that) const{
    z3::expr lhv = get();
    z3::expr rhv = that.get();
    return self(to_expr(lhv.ctx(), Z3_mk_implies(lhv.ctx(), lhv, rhv)),
            spliceAxioms(*this, that));
}

Bool Bool::iff(Bool that) const{
    z3::expr lhv = get();
    z3::expr rhv = that.get();
    return self(to_expr(lhv.ctx(), Z3_mk_iff(lhv.ctx(), lhv, rhv)),
            spliceAxioms(*this, that));
}

z3::expr Bool::toAxiom() const{
    return get() && axiom();
}

z3::sort Bool::sort(z3::context& ctx) {
    return ctx.bool_sort();
}

Bool Bool::mkBound(z3::context& ctx, unsigned i) {
    return z3::to_expr(ctx, Z3_mk_bound(ctx, i, sort(ctx)));
}

Bool Bool::mkConst(z3::context& ctx, bool value) {
    return ctx.bool_val(value);
}

Bool Bool::mkVar(z3::context& ctx, const std::string& name) {
    return ctx.bool_const(name.c_str());
}

Bool Bool::mkVar(z3::context& ctx, const std::string& name,
        std::function<Bool(Bool)> daAxiom) {
    // first construct the no-axiom version
    self cnst = mkVar(ctx, name);
    return self(cnst.get(),
            z3impl::spliceAxioms(cnst.axiom(), daAxiom(cnst).toAxiom()));
}

Bool Bool::mkFreshVar(z3::context& ctx, const std::string& name) {
    return self(ctx, Z3_mk_fresh_const(ctx, name.c_str(), sort(ctx)));
}

Bool Bool::mkFreshVar(z3::context& ctx, const std::string& name,
        std::function<Bool(Bool)> daAxiom) {
    // first construct the no-axiom version
    self cnst = mkFreshVar(ctx, name);
    return self(cnst.get(),
            z3impl::spliceAxioms(cnst.axiom(), daAxiom(cnst).toAxiom()));
}

std::ostream& operator << (std::ostream& ost, Bool b) {
    return ost << b.get().simplify() << " assuming " << b.axiom().simplify();
}

std::vector<BitVector<8>> splitBytes(SomeExpr bv) {
    typedef BitVector<8> Byte;

    size_t width = bv.get().get_sort().bv_size();

    z3::context& ctx = bv.get().ctx();

    std::vector<Byte> ret;

    for (size_t ix = 0; ix < width; ix += 8) {
        z3::expr e = z3::to_expr(ctx, Z3_mk_extract(ctx, ix+7, ix, bv.get()));
        ret.push_back(Byte(e, bv.axiom()));
    }

    return ret;
}

SomeExpr concatBytesDynamic(const std::vector<BitVector<8>>& bytes) {
    typedef BitVector<8> Byte;

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


} // namespace logic
} // namespace borealis
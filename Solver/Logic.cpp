/*
 * Logic.cpp
 *
 *  Created on: Dec 12, 2012
 *      Author: belyaev
 */

#include "Logic.hpp"

namespace borealis {
namespace logic {

ConversionException::ConversionException(const std::string& msg):
        std::exception(), message(msg) {}

const char* ConversionException::what() const throw() {
    return message.c_str();
}

struct ValueExpr::Impl {
    z3::expr inner;
    z3::expr axiomatic;
};

ValueExpr::ValueExpr(const ValueExpr& that):
    pimpl(new Impl(*that.pimpl)) {}

ValueExpr::ValueExpr(ValueExpr&& that):
    pimpl((that.pimpl)) {}

ValueExpr::ValueExpr(z3::expr e, z3::expr axiom):
    pimpl(new Impl{e, axiom}) {}

ValueExpr::ValueExpr(z3::expr e):
    pimpl(new Impl{e, z3impl::defaultAxiom(e)}) {}

ValueExpr::~ValueExpr() {
    delete pimpl;
}

void ValueExpr::swap(ValueExpr& that) {
    std::swap(pimpl, that.pimpl);
}

ValueExpr& ValueExpr::operator=(const ValueExpr& that) {
    ValueExpr e(that);
    swap(e);
    return *this;
}

z3::expr ValueExpr::get() const {
    return pimpl->inner;
}

z3::expr ValueExpr::axiom() const {
    return pimpl->axiomatic;
}

z3::sort ValueExpr::get_sort() const {
    return pimpl->inner.get_sort();
}

z3::context& ValueExpr::ctx() const {
    return pimpl->inner.ctx();
}

Bool::Bool(const Bool&) = default;

Bool::Bool(z3::expr e, z3::expr axiom) :
        ValueExpr(e, axiom) {
    if (!(e.is_bool())) {
        throw ConversionException(
                "Trying to construct Bool from non-bool: "
                        + borealis::util::toString(e));
    }
}

Bool::Bool(z3::expr e) :
        ValueExpr(e) {
    if (!(e.is_bool())) {
        throw ConversionException(
                "Trying to construct Bool from non-bool: "
                        + borealis::util::toString(e));
    }
}

Bool::Bool(z3::context& ctx, Z3_ast ast): Bool(z3::to_expr(ctx, ast)){};

Bool Bool::implies(Bool that) const{
    z3::expr lhv = get();
    z3::expr rhv = that.get();
    return self(to_expr(lhv.ctx(), Z3_mk_implies(lhv.ctx(), lhv, rhv)), spliceAxioms(*this, that));
}

Bool Bool::iff(Bool that) const{
    z3::expr lhv = get();
    z3::expr rhv = that.get();
    return self(to_expr(lhv.ctx(), Z3_mk_iff(lhv.ctx(), lhv, rhv)), spliceAxioms(*this, that));
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
    self nav = mkVar(ctx, name);
    return self(nav.get(), z3impl::spliceAxioms(nav.axiom(), daAxiom(nav).toAxiom()));
}

Bool Bool::mkFreshVar(z3::context& ctx, const std::string& name) {
    return self(ctx, Z3_mk_fresh_const(ctx, name.c_str(), sort(ctx)));
}

Bool Bool::mkFreshVar(z3::context& ctx, const std::string& name,
        std::function<Bool(Bool)> daAxiom) {
    // first construct the no-axiom version
    self nav = mkFreshVar(ctx, name);
    return self(nav.get(), z3impl::spliceAxioms(nav.axiom(), daAxiom(nav).toAxiom()));
}

std::ostream& operator<<(std::ostream& ost, Bool b) {
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

Bool operator!(Bool bv0) {
    return Bool(!bv0.get(), bv0.axiom());
}

} // namespace logic
} // namespace borealis

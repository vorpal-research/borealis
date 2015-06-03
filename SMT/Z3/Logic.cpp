/*
 * Logic.cpp
 *
 *  Created on: Dec 12, 2012
 *      Author: belyaev
 */

#include <sstream>

#include "SMT/Z3/Logic.hpp"

namespace borealis {
namespace z3_ {
namespace logic {

ConversionException::ConversionException(const std::string& msg) :
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
    pimpl(std::move(that.pimpl)) {}

ValueExpr::ValueExpr(z3::expr e, z3::expr axiom):
    pimpl(new Impl{e, axiom}) {}

ValueExpr::ValueExpr(z3::expr e):
    pimpl(new Impl{e, z3impl::defaultAxiom(e)}) {}

ValueExpr::ValueExpr(z3::context& ctx, Z3_ast e):
    pimpl(new Impl{z3::to_expr(ctx, e), z3impl::defaultAxiom(ctx)}) {}

ValueExpr::~ValueExpr() {}

void ValueExpr::swap(ValueExpr& that) {
    std::swap(pimpl, that.pimpl);
}

ValueExpr& ValueExpr::operator=(const ValueExpr& that) {
    ValueExpr e(that);
    swap(e);
    return *this;
}

std::string ValueExpr::getName() const {
    return z3impl::getName(this);
}

std::string ValueExpr::toSmtLib() const {
    return z3impl::asSmtLib(this);
}

ValueExpr ValueExpr::simplify() const {
    z3::params params(this->pimpl->inner.ctx());

    params.set(":pull-cheap-ite", true);
    params.set(":push-bv-ite", false);
    params.set(":expand-select-store", true);

    return ValueExpr{
        this->pimpl->inner.simplify(params),
        this->pimpl->axiomatic.simplify(params)
    };
}

namespace z3impl {
    z3::expr getExpr(const ValueExpr& a) {
        return a.pimpl->inner;
    }

    z3::expr getAxiom(const ValueExpr& a) {
        return a.pimpl->axiomatic;
    }

    z3::sort getSort(const ValueExpr& a) {
        return a.pimpl->inner.get_sort();
    }

    z3::context& getContext(const ValueExpr& a) {
        return a.pimpl->inner.ctx();
    }

    std::string getName(const ValueExpr& a) {
        std::ostringstream oss;
        oss << getExpr(a).decl().name();
        return oss.str();
    }

    z3::expr asAxiom(const ValueExpr& a) {
        return a.pimpl->inner && a.pimpl->axiomatic;
    }

    std::string asSmtLib(const ValueExpr& a) {
        std::ostringstream oss;
        oss << getExpr(a) << "\n" << getAxiom(a);
        return oss.str();
    }
} // namespace z3impl

std::ostream& operator<<(std::ostream& ost, const ValueExpr& v) {
    return ost << z3impl::getExpr(v) << " assuming " << z3impl::getAxiom(v);
}

Bool implies(Bool lhv, Bool rhv) {
    z3::expr lhv_raw = z3impl::getExpr(lhv);
    z3::expr rhv_raw = z3impl::getExpr(rhv);
    auto& ctx = lhv_raw.ctx();

    return Bool{
        z3::to_expr(ctx, Z3_mk_implies(ctx, lhv_raw, rhv_raw)),
        z3impl::spliceAxioms(lhv, rhv)
    };
}

Bool iff(Bool lhv, Bool rhv) {
    z3::expr lhv_raw = z3impl::getExpr(lhv);
    z3::expr rhv_raw = z3impl::getExpr(rhv);
    auto& ctx = lhv_raw.ctx();

    return Bool{
        z3::to_expr(ctx, Z3_mk_iff(ctx, lhv_raw, rhv_raw)),
        z3impl::spliceAxioms(lhv, rhv)
    };
}


#define REDEF_BOOL_BOOL_OP(OP) \
        Bool operator OP(Bool bv0, Bool bv1) { \
            return Bool{ \
                z3impl::getExpr(bv0) OP z3impl::getExpr(bv1), \
                z3impl::spliceAxioms(bv0, bv1) \
            }; \
        }

REDEF_BOOL_BOOL_OP(==)
REDEF_BOOL_BOOL_OP(!=)
REDEF_BOOL_BOOL_OP(&&)
REDEF_BOOL_BOOL_OP(||)

Bool operator^(Bool bv0, Bool bv1) {
    z3::expr bv0_raw = z3impl::getExpr(bv0);
    z3::expr bv1_raw = z3impl::getExpr(bv1);
    auto& ctx = bv0_raw.ctx();

    return Bool{
        z3::to_expr(ctx, Z3_mk_xor(ctx, bv0_raw, bv1_raw)),
        z3impl::spliceAxioms(bv0, bv1)
    };
}

#undef REDEF_BOOL_BOOL_OP


Bool operator!(Bool bv0) {
    return Bool{ !z3impl::getExpr(bv0), z3impl::getAxiom(bv0) };
}


#define REDEF_OP(OP) \
    Bool operator OP(const ComparableExpr& lhv, const ComparableExpr& rhv) { \
        return Bool{ \
            z3impl::getExpr(lhv) OP z3impl::getExpr(rhv), \
            z3impl::spliceAxioms(lhv, rhv) \
        }; \
    }

    REDEF_OP(<)
    REDEF_OP(>)
    REDEF_OP(>=)
    REDEF_OP(<=)
    REDEF_OP(==)
    REDEF_OP(!=)

#undef REDEF_OP


#define BIN_OP(OP) \
    DynBitVectorExpr operator OP(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv) { \
        size_t sz = std::max(lhv.getBitSize(), rhv.getBitSize()); \
        DynBitVectorExpr dlhv = lhv.growTo(sz); \
        DynBitVectorExpr drhv = rhv.growTo(sz); \
        return DynBitVectorExpr{ \
            z3impl::getExpr(dlhv) OP z3impl::getExpr(drhv), \
            z3impl::spliceAxioms(lhv, rhv) \
        }; \
    }

    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)
    BIN_OP(|)
    BIN_OP(&)
    BIN_OP(^)

#undef BIN_OP

DynBitVectorExpr operator%(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv) {
    size_t sz = std::max(lhv.getBitSize(), rhv.getBitSize());
    DynBitVectorExpr dlhv = lhv.growTo(sz);
    DynBitVectorExpr drhv = rhv.growTo(sz);
    auto& ctx = z3impl::getContext(lhv);

    auto res = z3::to_expr(ctx, Z3_mk_bvsrem(ctx, z3impl::getExpr(dlhv), z3impl::getExpr(drhv)));
    auto axm = z3impl::spliceAxioms(lhv, rhv);
    return DynBitVectorExpr{ res, axm };
}

DynBitVectorExpr operator>>(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv) {
    size_t sz = std::max(lhv.getBitSize(), rhv.getBitSize());
    DynBitVectorExpr dlhv = lhv.growTo(sz);
    DynBitVectorExpr drhv = rhv.growTo(sz);
    auto& ctx = z3impl::getContext(lhv);

    auto res = z3::to_expr(ctx, Z3_mk_bvashr(ctx, z3impl::getExpr(dlhv), z3impl::getExpr(drhv)));
    auto axm = z3impl::spliceAxioms(lhv, rhv);
    return DynBitVectorExpr{ res, axm };
}

DynBitVectorExpr operator<<(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv) {
    size_t sz = std::max(lhv.getBitSize(), rhv.getBitSize());
    DynBitVectorExpr dlhv = lhv.growTo(sz);
    DynBitVectorExpr drhv = rhv.growTo(sz);
    auto& ctx = z3impl::getContext(lhv);

    auto res = z3::to_expr(ctx, Z3_mk_bvshl(ctx, z3impl::getExpr(dlhv), z3impl::getExpr(drhv)));
    auto axm = z3impl::spliceAxioms(lhv, rhv);
    return DynBitVectorExpr{ res, axm };
}

} // namespace logic
} // namespace z3_
} // namespace borealis

/*
 * Logic.cpp
 *
 *  Created on: Jul 31, 2013
 *      Author: Sam Kolton
 */

#include <sstream>

#include "SMT/MathSAT/Logic.hpp"

namespace borealis {
namespace mathsat_ {
namespace logic {

struct ValueExpr::Impl {
    mathsat::Expr inner;
    mathsat::Expr axiomatic;
};

ValueExpr::ValueExpr(const ValueExpr& that):
    pimpl(new Impl(*that.pimpl)) {}

ValueExpr::ValueExpr(ValueExpr&& that):
    pimpl(std::move(that.pimpl)) {}

ValueExpr::ValueExpr(mathsat::Expr e, mathsat::Expr axiom):
    pimpl(new Impl{e, axiom}) {}

ValueExpr::ValueExpr(mathsat::Expr e):
    pimpl(new Impl{e, msatimpl::defaultAxiom(e)}) {}

ValueExpr::ValueExpr(mathsat::Env& env, msat_term e):
    pimpl(new Impl{mathsat::Expr(env, e), msatimpl::defaultAxiom(env)}) {}

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
    return msatimpl::getName(this);
}

std::string ValueExpr::toSmtLib() const {
    return msatimpl::asSmtLib(this);
}

namespace msatimpl {
    mathsat::Expr getExpr(const ValueExpr& a) {
        return a.pimpl->inner;
    }

    mathsat::Expr getAxiom(const ValueExpr& a) {
        return a.pimpl->axiomatic;
    }

    mathsat::Sort getSort(const ValueExpr& a) {
        return a.pimpl->inner.get_sort();
    }

    const mathsat::Env& getEnvironment(const ValueExpr& a) {
        return a.pimpl->inner.env();
    }

    std::string getName(const ValueExpr& a) {
        return a.pimpl->inner.decl().name();
    }

    mathsat::Expr asAxiom(const ValueExpr& a) {
        return a.pimpl->inner && a.pimpl->axiomatic;
    }

    std::string asSmtLib(const ValueExpr& a) {
        std::ostringstream oss;
        oss << getExpr(a) << "\n" << getAxiom(a);
        return oss.str();
    }
} // namespace msatimpl

std::ostream& operator<<(std::ostream& ost, const ValueExpr& v) {
    return ost << msatimpl::getExpr(v) << " assuming " << msatimpl::getAxiom(v);
}

////////////////////////////////////////////////////////////////////////////////

Bool implies(Bool lhv, Bool rhv) {
    mathsat::Expr lhv_raw = msatimpl::getExpr(lhv);
    mathsat::Expr rhv_raw = msatimpl::getExpr(rhv);
    return Bool(mathsat::implies(lhv_raw, rhv_raw),
                msatimpl::spliceAxioms(lhv, rhv));
}

Bool iff(Bool lhv, Bool rhv) {
    mathsat::Expr lhv_raw = msatimpl::getExpr(lhv);
    mathsat::Expr rhv_raw = msatimpl::getExpr(rhv);
    return Bool(mathsat::iff(lhv_raw, rhv_raw),
                msatimpl::spliceAxioms(lhv, rhv));
}


#define REDEF_BOOL_BOOL_OP(OP) \
        Bool operator OP(Bool bv0, Bool bv1) { \
            return Bool{ \
                msatimpl::getExpr(bv0) OP msatimpl::getExpr(bv1), \
                msatimpl::spliceAxioms(bv0, bv1) \
            }; \
        }

REDEF_BOOL_BOOL_OP(==)
REDEF_BOOL_BOOL_OP(!=)
REDEF_BOOL_BOOL_OP(&&)
REDEF_BOOL_BOOL_OP(||)
REDEF_BOOL_BOOL_OP(^)

#undef REDEF_BOOL_BOOL_OP


Bool operator!(Bool bv0) {
    return Bool{ !msatimpl::getExpr(bv0), msatimpl::getAxiom(bv0) };
}

////////////////////////////////////////////////////////////////////////////////

#define REDEF_OP(OP) \
    Bool operator OP(const ComparableExpr& lhv, const ComparableExpr& rhv) { \
        return Bool{ \
            msatimpl::getExpr(lhv) OP msatimpl::getExpr(rhv), \
            msatimpl::spliceAxioms(lhv, rhv) \
        }; \
    }

    REDEF_OP(<)
    REDEF_OP(>)
    REDEF_OP(>=)
    REDEF_OP(<=)
    REDEF_OP(==)
    REDEF_OP(!=)

#undef REDEF_OP

////////////////////////////////////////////////////////////////////////////////

#define BIN_OP(OP) \
    DynBitVectorExpr operator OP(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv) { \
        size_t sz = std::max(lhv.getBitSize(), rhv.getBitSize()); \
        DynBitVectorExpr dlhv = lhv.growTo(sz); \
        DynBitVectorExpr drhv = rhv.growTo(sz); \
        return DynBitVectorExpr{ \
            msatimpl::getExpr(dlhv) OP msatimpl::getExpr(drhv), \
            msatimpl::spliceAxioms(lhv, rhv) \
        }; \
    }

    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)
    BIN_OP(|)
    BIN_OP(&)
    BIN_OP(^)
    BIN_OP(%)

#undef BIN_OP

DynBitVectorExpr operator>>(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv) {
    size_t sz = std::max(lhv.getBitSize(), rhv.getBitSize());
    DynBitVectorExpr dlhv = lhv.growTo(sz);
    DynBitVectorExpr drhv = rhv.growTo(sz);

    auto res = mathsat::ashr(msatimpl::getExpr(dlhv), msatimpl::getExpr(drhv));
    auto axm = msatimpl::spliceAxioms(lhv, rhv);
    return DynBitVectorExpr{ res, axm };
}

DynBitVectorExpr operator<<(const DynBitVectorExpr& lhv, const DynBitVectorExpr& rhv) {
    size_t sz = std::max(lhv.getBitSize(), rhv.getBitSize());
    DynBitVectorExpr dlhv = lhv.growTo(sz);
    DynBitVectorExpr drhv = rhv.growTo(sz);

    auto res = mathsat::lshl(msatimpl::getExpr(dlhv), msatimpl::getExpr(drhv));
    auto axm = msatimpl::spliceAxioms(lhv, rhv);
    return DynBitVectorExpr{ res, axm };
}

} // namespace logic
} // namespace mathsat
} // namespace borealis

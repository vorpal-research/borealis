/*
 * Unlogic.cpp
 *
 *  Created on: Feb 11, 2014
 *      Author: sam
 */

#include "Factory/Nest.h"
#include "SMT/Z3/Unlogic/Unlogic.h"

#include "Util/macros.h"

namespace borealis {
namespace z3_ {
namespace unlogic {

USING_SMT_LOGIC(Z3);

Term::Ptr undoBv(const z3::expr& expr, const FactoryNest& FN) {
    unsigned long long i;
    auto res = Z3_get_numeral_uint64(expr.ctx(), expr, &i);
    ASSERT(res != 0, "Something bad occurs while getting int value from Z3 expression");
    return FN.Term->getIntTerm(i, expr.get_sort().bv_size(), llvm::Signedness::Unknown);
}

Term::Ptr undoBool(const z3::expr& expr, const FactoryNest& FN) {
    auto res = Z3_get_bool_value(expr.ctx(), expr);
    if (res == Z3_L_TRUE) {
        return FN.Term->getTrueTerm();
    } else if (res == Z3_L_FALSE) {
        return FN.Term->getFalseTerm();
    } else {
        // FIXME: think about dealing with bool unknowns
        BYE_BYE(Term::Ptr, "Trying to unbool z3::unknown");
    }
}

Term::Ptr undoReal(const z3::expr& expr, const FactoryNest& FN) {
    long long n, d;
    auto res = Z3_get_numeral_rational_int64(expr.ctx(), expr, &n, &d);
    if (res != 0) {
        double dn = n;
        double dd = d;
        return FN.Term->getOpaqueConstantTerm(dn / dd);
    } else {
        // FIXME: what to do when we can't get the rational from Z3?
        return FN.Term->getOpaqueConstantTerm(std::numeric_limits<double>::max());
    }
}

Term::Ptr undoThat(Z3::Dynamic dyn) {
    FactoryNest FN;

    auto&& expr = logic::z3impl::getExpr(dyn);
    ASSERT(expr.is_numeral() || expr.is_bool(), "Trying to unlogic non-numeral or bool");

    if (expr.is_bv()) {
        return undoBv(expr, FN);
    } else if (expr.is_real()) {
        return undoReal(expr, FN);
    } else if (expr.is_bool()) {
        return undoBool(expr, FN);
    }

    BYE_BYE(Term::Ptr, "Unsupported numeral type");
}

} // namespace unlogic
} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"

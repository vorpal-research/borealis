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

Term::Ptr undoBv(Term::Ptr witness, const z3::expr& expr, const FactoryNest& FN) {
    auto res = Z3_get_numeral_string(expr.ctx(), expr);
    auto size = expr.get_sort().bv_size();

    return FN.Term->getIntTerm(res, size, llvm::Signedness::Unknown)->setType(FN.Term.get(), witness->getType());
}

Term::Ptr undoBool(Term::Ptr /* witness */, const z3::expr& expr, const FactoryNest& FN) {
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

Term::Ptr undoReal(Term::Ptr witness, const z3::expr& expr, const FactoryNest& FN) {
    int64_t n, d;
    auto res = Z3_get_numeral_rational_int64(expr.ctx(), expr, &n, &d);
    if (res != 0) {
        double dn = n;
        double dd = d;
        return FN.Term->getOpaqueConstantTerm(dn / dd)->setType(FN.Term.get(), witness->getType());
    } else {
        // FIXME: what to do when we can't get the rational from Z3?
        return FN.Term->getOpaqueConstantTerm(std::numeric_limits<double>::max())->setType(FN.Term.get(), witness->getType());
    }
}

Term::Ptr undoThat(const FactoryNest& FN, Term::Ptr witness, Z3::Dynamic dyn) {
    auto&& expr = dyn.getExpr();
    ASSERT(expr.is_numeral() || expr.is_bool(), "Trying to unlogic non-numeral or bool");

    if (expr.is_bv()) {
        return undoBv(witness, expr, FN);
    } else if (expr.is_real()) {
        return undoReal(witness, expr, FN);
    } else if (expr.is_bool()) {
        return undoBool(witness, expr, FN);
    }

    BYE_BYE(Term::Ptr, "Unsupported numeral type");
}

} // namespace unlogic
} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"

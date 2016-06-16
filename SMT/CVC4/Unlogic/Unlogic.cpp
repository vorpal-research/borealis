/*
 * Unlogic.cpp
 *
 *  Created on: Feb 11, 2014
 *      Author: sam
 */

#include "Factory/Nest.h"
#include "SMT/CVC4/Unlogic/Unlogic.h"

#include "Util/macros.h"

namespace borealis {
namespace cvc4_ {
namespace unlogic {

USING_SMT_LOGIC(CVC4);

Term::Ptr undoBv(const ::CVC4::Expr& bv, const FactoryNest& FN) {
    ASSERTC(bv.isConst());
    auto cnst = bv.template getConst<::CVC4::BitVector>();
    auto&& iv = cnst.getValue();
    auto size = cnst.getSize();

    return FN.Term->getIntTerm(iv.toString(10), size, llvm::Signedness::Unknown);
}

Term::Ptr undoBool(const ::CVC4::Expr& b, const FactoryNest& FN) {
    ASSERTC(b.isConst());
    return FN.Term->getOpaqueConstantTerm(b.template getConst<bool>());
}

Term::Ptr undoReal(const ::CVC4::Expr& bv, const FactoryNest& FN) {
    ASSERTC(bv.isConst());
    ::CVC4::Rational cnst = bv.template getConst<::CVC4::Rational>();

    return FN.Term->getOpaqueConstantTerm(cnst.getDouble());

}

Term::Ptr undoThat(CVC4::Dynamic dyn) {
    FactoryNest FN;

    ::CVC4::Expr expr = dyn.getExpr();
    auto type = expr.getType(false);

    if (type.isBitVector()) {
        return undoBv(expr, FN);
    } else if (type.isReal()) {
        return undoReal(expr, FN);
    } else if (type.isBoolean()) {
        return undoBool(expr, FN);
    }

    BYE_BYE(Term::Ptr, "Unsupported numeral type");
}

} // namespace unlogic
} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"

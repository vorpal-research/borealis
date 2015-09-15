/*
 * Symbols.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: sam
 */

#include "SMT/MathSAT/Unlogic/Symbols.h"

#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {
namespace unlogic {

AbstractSymbol::Ptr SymbolFactory(const mathsat::Expr& expr) {
    if (expr.num_args() == 0) {
        if (msat_term_is_true(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new TrueSymbol(expr) };
        } else if (msat_term_is_false(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new FalseSymbol(expr) };
        } else if (msat_term_is_uf(expr.env(), expr)) {
            BYE_BYE(AbstractSymbol::Ptr, "Can't unlogic UF");
        } else if (msat_term_is_number(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new ConstantSymbol(expr) };
        } else if (msat_term_is_constant(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new ValueSymbol(expr) };
        } else {
            BYE_BYE(AbstractSymbol::Ptr, "Unknown 0-arg expr type");
        }
    }

    auto tag = expr.decl().tag();
    switch (tag) {
    case MSAT_TAG_ITE:
        return AbstractSymbol::Ptr{ new IteOperationSymbol(expr) };

    case MSAT_TAG_IFF:
        return AbstractSymbol::Ptr{ new IffOperationSymbol(expr) };

    case MSAT_TAG_NOT:
    case MSAT_TAG_BV_NOT:
    case MSAT_TAG_BV_NEG:
        return AbstractSymbol::Ptr{ new UnaryOperationSymbol(tag, expr) };

    case MSAT_TAG_AND:
    case MSAT_TAG_OR:
    case MSAT_TAG_TIMES:
    case MSAT_TAG_BV_MUL:
    case MSAT_TAG_PLUS:
    case MSAT_TAG_BV_ADD:
    case MSAT_TAG_BV_AND:
    case MSAT_TAG_BV_OR:
    case MSAT_TAG_BV_XOR:
    case MSAT_TAG_BV_SUB:
    case MSAT_TAG_BV_UDIV:
    case MSAT_TAG_BV_SDIV:
    case MSAT_TAG_BV_UREM:
    case MSAT_TAG_BV_SREM:
    case MSAT_TAG_BV_LSHL:
    case MSAT_TAG_BV_LSHR:
    case MSAT_TAG_BV_ASHR:
        return AbstractSymbol::Ptr{ new BinaryOperationSymbol(tag, expr) };

    case MSAT_TAG_EQ:
    case MSAT_TAG_LEQ:
    case MSAT_TAG_BV_SLE:
    case MSAT_TAG_BV_SLT:
    case MSAT_TAG_BV_ULE:
    case MSAT_TAG_BV_ULT:
        return AbstractSymbol::Ptr{ new CmpOperationSymbol(tag, expr) };

    case MSAT_TAG_BV_EXTRACT:
        return AbstractSymbol::Ptr{ new ExtractSymbol(expr) };

    default:
        if (msat_term_is_uf(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new LoadSymbol(expr) };
        }

        borealis::logging::wtf() << "Unknown expr: " << expr << endl;
        BYE_BYE(AbstractSymbol::Ptr, "Unknown expr type");
    }
}

} // namespace unlogic
} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"

//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_UTILS_HPP
#define BOREALIS_UTILS_HPP

#include <llvm/ADT/APSInt.h>

#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

static llvm::APInt min(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    if (isSigned) return lhv.slt(rhv) ? lhv : rhv;
    else return lhv.ult(rhv) ? lhv : rhv;
}

static llvm::APInt max(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    if (isSigned) return lhv.sgt(rhv) ? lhv : rhv;
    else return lhv.ugt(rhv) ? lhv : rhv;
}

static bool less(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return lhv.compare(rhv) == llvm::APFloat::cmpLessThan;
}

static bool equals(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return lhv.compare(rhv) == llvm::APFloat::cmpEqual;
}

static bool greater(const llvm::APFloat& lhv, const llvm::APFloat rhv) {
    return lhv.compare(rhv) == llvm::APFloat::cmpGreaterThan;
}

static llvm::APFloat min(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return less(lhv, rhv) ? lhv : rhv;
}

static llvm::APFloat max(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return less(lhv, rhv) ? rhv : lhv;
}

static bool lt(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    return isSigned ?
           lhv.slt(rhv) :
           rhv.ult(rhv);
}

static bool le(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    return isSigned ?
           lhv.sle(rhv) :
           rhv.ule(rhv);
}

static bool gt(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    return isSigned ?
           lhv.sgt(rhv) :
           rhv.ugt(rhv);
}

static bool ge(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    return isSigned ?
           lhv.sge(rhv) :
           rhv.uge(rhv);
}

static const llvm::fltSemantics& getSemantics(const llvm::Type& type) {
    ASSERTC(type.isFloatingPointTy());
    if (type.isHalfTy())
        return llvm::APFloat::IEEEhalf;
    else if (type.isFloatTy())
        return llvm::APFloat::IEEEsingle;
    else if (type.isDoubleTy())
        return llvm::APFloat::IEEEdouble;
    else if (type.isFP128Ty())
        return llvm::APFloat::IEEEquad;
    else if (type.isPPC_FP128Ty())
        return llvm::APFloat::PPCDoubleDouble;
    else if (type.isX86_FP80Ty())
        return llvm::APFloat::x87DoubleExtended;

    UNREACHABLE("Unable to get semantics of random type");
}

}   /* namespace util */
}   /* namespace borealis */

#include "Util/unmacros.h"

#endif //BOREALIS_UTILS_HPP

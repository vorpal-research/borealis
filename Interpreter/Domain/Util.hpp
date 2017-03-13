//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_UTILS_HPP
#define BOREALIS_UTILS_HPP

#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/Hashing.h>

#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

static llvm::APInt getMaxValue(unsigned width, bool isSigned = false) {
    return isSigned ? llvm::APInt::getSignedMaxValue(width) : llvm::APInt::getMaxValue(width);
}

static llvm::APInt getMinValue(unsigned width, bool isSigned = false) {
    return isSigned ? llvm::APInt::getSignedMinValue(width) : llvm::APInt::getMinValue(width);
}

static llvm::APInt min(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false) {
    if (isSigned) return lhv.slt(rhv) ? lhv : rhv;
    else return lhv.ult(rhv) ? lhv : rhv;
}

static llvm::APInt max(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false) {
    if (isSigned) return lhv.sgt(rhv) ? lhv : rhv;
    else return lhv.ugt(rhv) ? lhv : rhv;
}

static bool lt(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return false;
    return lhv.compare(rhv) == llvm::APFloat::cmpLessThan;
}

static bool eq(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return true;
    return lhv.compare(rhv) == llvm::APFloat::cmpEqual;
}

static bool gt(const llvm::APFloat& lhv, const llvm::APFloat rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return false;
    return lhv.compare(rhv) == llvm::APFloat::cmpGreaterThan;
}

static llvm::APFloat min(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return lt(lhv, rhv) ? lhv : rhv;
}

static llvm::APFloat max(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return lt(lhv, rhv) ? rhv : lhv;
}

static bool eq(const llvm::APInt& lhv, const llvm::APInt& rhv) {
    if (lhv.getBitWidth() != rhv.getBitWidth()) return false;
    return lhv.eq(rhv);
}

static bool lt(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false) {
    return isSigned ?
           lhv.slt(rhv) :
           rhv.ult(rhv);
}

static bool le(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false) {
    return isSigned ?
           lhv.sle(rhv) :
           lhv.ule(rhv);
}

static bool gt(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false) {
    return isSigned ?
           lhv.sgt(rhv) :
           rhv.ugt(rhv);
}

static bool ge(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false) {
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

namespace std {

template <>
struct hash<llvm::APFloat> {
    size_t operator() (const llvm::APFloat& apFloat) const noexcept {
        return llvm::hash_value(apFloat);
    }
};

template <>
struct hash<llvm::APInt> {
    size_t operator() (const llvm::APInt& apInt) const noexcept {
        return llvm::hash_value(apInt);
    }
};

}   /* namespace std */

#include "Util/unmacros.h"

#endif //BOREALIS_UTILS_HPP

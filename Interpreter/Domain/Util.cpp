//
// Created by abdullin on 3/16/17.
//


#include "Util.h"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace util {

///////////////////////////////////////////////////////////////
/// APInt util
///////////////////////////////////////////////////////////////

llvm::APInt getMaxValue(unsigned width, bool isSigned) {
    return isSigned ? llvm::APInt::getSignedMaxValue(width) : llvm::APInt::getMaxValue(width);
}

llvm::APInt getMinValue(unsigned width, bool isSigned) {
    return isSigned ? llvm::APInt::getSignedMinValue(width) : llvm::APInt::getMinValue(width);
}

bool isMaxValue(const llvm::APInt& val, bool isSigned) {
    return isSigned ?
           val.isMaxSignedValue() :
           val.isMaxValue();
}

bool isMinValue(const llvm::APInt& val, bool isSigned) {
    return isSigned ?
           val.isMinSignedValue() :
           val.isMinValue();
}

llvm::APInt min(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    if (isSigned) return lhv.slt(rhv) ? lhv : rhv;
    else return lhv.ult(rhv) ? lhv : rhv;
}

llvm::APInt max(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    if (isSigned) return lhv.sgt(rhv) ? lhv : rhv;
    else return lhv.ugt(rhv) ? lhv : rhv;
}

bool eq(const llvm::APInt& lhv, const llvm::APInt& rhv) {
    if (lhv.getBitWidth() != rhv.getBitWidth()) return false;
    return lhv.eq(rhv);
}

bool lt(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    return isSigned ?
           lhv.slt(rhv) :
           lhv.ult(rhv);
}

bool le(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    return isSigned ?
           lhv.sle(rhv) :
           lhv.ule(rhv);
}

bool gt(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    return isSigned ?
           lhv.sgt(rhv) :
           lhv.ugt(rhv);
}

bool ge(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned) {
    return isSigned ?
           lhv.sge(rhv) :
           lhv.uge(rhv);
}

std::string toString(const llvm::APInt& val, bool isSigned) {
    llvm::SmallVector<char, 32> valVector;
    if (isSigned) val.toStringSigned(valVector, 10);
    else val.toStringUnsigned(valVector, 10);

    std::ostringstream ss;
    for (auto&& it : valVector) ss << it;
    return std::move(ss.str());
}

///////////////////////////////////////////////////////////////
/// APFloat util
///////////////////////////////////////////////////////////////

llvm::APFloat getMaxValue(const llvm::fltSemantics& semantics) {
    return llvm::APFloat::getInf(semantics, false);
}

llvm::APFloat getMinValue(const llvm::fltSemantics& semantics) {
    return llvm::APFloat::getInf(semantics, true);
}

llvm::APFloat min(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return lt(lhv, rhv) ? lhv : rhv;
}

llvm::APFloat max(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return lt(lhv, rhv) ? rhv : lhv;
}

bool lt(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return false;
    return lhv.compare(rhv) == llvm::APFloat::cmpLessThan;
}

bool eq(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return true;
    return lhv.compare(rhv) == llvm::APFloat::cmpEqual;
}

bool gt(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return false;
    return lhv.compare(rhv) == llvm::APFloat::cmpGreaterThan;
}

const llvm::fltSemantics& getSemantics(const llvm::Type& type) {
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

std::string toString(const llvm::APFloat& val) {
    llvm::SmallVector<char, 32> valVector;
    val.toString(valVector, 8);

    std::ostringstream ss;
    for (auto&& it : valVector) ss << it;
    return std::move(ss.str());
}

}   /* namespace util */
}   /* namespace borealis */

#include "Util/unmacros.h"

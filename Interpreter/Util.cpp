//
// Created by abdullin on 3/16/17.
//


#include <llvm/Support/raw_ostream.h>

#include "Interpreter/Domain/Integer/MaxInteger.h"
#include "Interpreter/Domain/Integer/MinInteger.h"
#include "Util.h"
#include "Util/ir_writer.h"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace util {

///////////////////////////////////////////////////////////////
/// APInt util
///////////////////////////////////////////////////////////////

absint::Integer::Ptr getMaxValue(size_t width) {
    return Integer::Ptr{ new MaxInteger(width) };
}

absint::Integer::Ptr getMinValue(size_t width) {
    return Integer::Ptr{ new MinInteger(width) };
}

absint::Integer::Ptr min(Integer::Ptr lhv, Integer::Ptr rhv, bool isSigned) {
    if (isSigned) return lhv->slt(rhv) ? lhv : rhv;
    else return lhv->lt(rhv) ? lhv : rhv;
}

absint::Integer::Ptr max(Integer::Ptr lhv, Integer::Ptr rhv, bool isSigned) {
    if (isSigned) return lhv->sgt(rhv) ? lhv : rhv;
    else return lhv->gt(rhv) ? lhv : rhv;
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

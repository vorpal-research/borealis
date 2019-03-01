//
// Created by abdullin on 3/16/17.
//


#include <llvm/Support/raw_ostream.h>
#include <Logging/logger.hpp>

#include "Interpreter/Domain/Numerical/Number.hpp"
#include "Util.hpp"
#include "Util/algorithm.hpp"
#include "Util/collections.hpp"
#include "Util/ir_writer.h"
#include "Util/sayonara.hpp"
#include "Util/streams.hpp"
#include "Util/macros.h"

namespace borealis {
namespace util {

llvm::APFloat getMaxValue(const llvm::fltSemantics& semantics) {
    return llvm::APFloat::getInf(semantics, false);
}

llvm::APFloat getMinValue(const llvm::fltSemantics& semantics) {
    return llvm::APFloat::getInf(semantics, true);
}

bool lt(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return false;
    return lhv.compare(rhv) == llvm::APFloat::cmpLessThan;
}

bool eq(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return true;
    return lhv.compare(rhv) == llvm::APFloat::cmpEqual;
}

bool le(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return true;
    auto cmp = lhv.compare(rhv);
    return cmp == llvm::APFloat::cmpEqual || cmp == llvm::APFloat::cmpLessThan;
}

bool gt(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return false;
    return lhv.compare(rhv) == llvm::APFloat::cmpGreaterThan;
}

bool ge(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    if (lhv.isNaN() && rhv.isNaN()) return true;
    auto cmp = lhv.compare(rhv);
    return cmp == llvm::APFloat::cmpEqual || cmp == llvm::APFloat::cmpGreaterThan;
}

const llvm::fltSemantics& getSemantics() {
    return absint::Float::getLlvmSemantics();
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

std::string toString(const llvm::APInt& val, bool isSigned) {
    llvm::SmallVector<char, 32> valVector;
    if (isSigned) val.toStringSigned(valVector, 10);
    else val.toStringUnsigned(valVector, 10);

    std::ostringstream ss;
    for (auto&& it : valVector) ss << it;
    return std::move(ss.str());
}

bool llvm_types_eq(const llvm::Type* lhv, const llvm::Type* rhv) {
    if (lhv == rhv) return true;
    if (lhv->getTypeID() != rhv->getTypeID()) return false;
    switch (lhv->getTypeID()) {
        // floats - all equal if ID is equal
        case llvm::Type::VoidTyID:
        case llvm::Type::HalfTyID:
        case llvm::Type::FloatTyID:
        case llvm::Type::DoubleTyID:
        case llvm::Type::X86_FP80TyID:
        case llvm::Type::FP128TyID:
        case llvm::Type::PPC_FP128TyID:
            return true;
        case llvm::Type::IntegerTyID:
            return lhv->getIntegerBitWidth() == rhv->getIntegerBitWidth();
        case llvm::Type::FunctionTyID: {
            auto* lhvf = llvm::cast<llvm::FunctionType>(lhv);
            auto* rhvf = llvm::cast<llvm::FunctionType>(rhv);
            if (not llvm_types_eq(lhvf->getReturnType(), rhvf->getReturnType())) return false;
            if (not lhvf->isVarArg() && not rhvf->isVarArg() && lhvf->getNumParams() != rhvf->getNumParams()) return false;
            for (auto i = 0U; i < util::min(lhvf->getNumParams(), rhvf->getNumParams()); ++i) {
                if (not llvm_types_eq(lhvf->getParamType(i), rhvf->getParamType(i))) return false;
            }
            return true;
        }
        case llvm::Type::StructTyID: {
            auto* lhvs = llvm::cast<llvm::StructType>(lhv);
            auto* rhvs = llvm::cast<llvm::StructType>(rhv);
            return lhvs->isLayoutIdentical(const_cast<llvm::StructType*>(rhvs));
        }
        case llvm::Type::ArrayTyID:
            if (lhv->getArrayNumElements() != rhv->getArrayNumElements()) return false;
            return llvm_types_eq(lhv->getArrayElementType(), rhv->getArrayElementType());
        case llvm::Type::PointerTyID:
            return llvm_types_eq(lhv->getPointerElementType(), rhv->getPointerElementType());
        case llvm::Type::VectorTyID:
            if (lhv->getVectorNumElements() != rhv->getVectorNumElements()) return false;
            return llvm_types_eq(lhv->getVectorElementType(), rhv->getVectorElementType());
        default:
            UNREACHABLE("Unknown TypeID");
    }
}

llvm::APFloat normalizeFloat(const llvm::APFloat& n) {
    llvm::APFloat result(n);
    bool b;
    result.convert(getSemantics(), absint::Float::getRoundingMode(), &b);
    return std::move(result);
}

}   /* namespace util */
}   /* namespace borealis */

#include "Util/unmacros.h"

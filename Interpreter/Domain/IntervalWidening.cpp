//
// Created by abdullin on 6/26/17.
//

#include "Interpreter/Domain/DomainFactory.h"
#include "Interpreter/Util.hpp"
#include "Interpreter/Domain/IntervalWidening.hpp"

namespace borealis {
namespace absint {

Integer::Ptr IntegerWidening::get_prev(const Integer::Ptr& value, DomainFactory*) const {
    auto width = value->getWidth();
    return Integer::getMinValue(width);
//
//    Integer::Ptr next;
//    auto ten = factory->toInteger(10, width);
//    if (value->getValue() == 0) {
//        next = Integer::getMinValue(width);
//    } else {
//        next = value->udiv(ten);
//    }
//    return next ? next : Integer::getMinValue(width);
}

Integer::Ptr IntegerWidening::get_next(const Integer::Ptr& value, DomainFactory*) const {
    auto width = value->getWidth();
    return Integer::getMaxValue(width);
//
//    Integer::Ptr next;
//    auto ten = factory->toInteger(10, width);
//    if (value->getValue() == 0) {
//        next = ten;
//    } else {
//        next = value->mul(ten);
//    }
//    return next ? next : Integer::getMaxValue(width);
}

Integer::Ptr IntegerWidening::get_signed_prev(const Integer::Ptr& value, DomainFactory*) const {
    if (not value->isValue()) return value;
    auto width = value->getWidth();
    return Integer::getMinValue(width);
//
//    Integer::Ptr next;
//    auto ten = factory->toInteger(10, width);
//    auto zero = factory->toInteger(0, width);
//    if (value->getValue() == llvm::APInt::getSignedMinValue(width)) {
//        next = Integer::getMinValue(width);
//    } else {
//        if (value->sgt(ten)) {
//            next = value->sdiv(ten);
//        } else if (value->sgt(zero)) {
//            next = value->mul(factory->toInteger(-1, width, true));
//        } else {
//            next = value->mul(ten);
//        }
//    }
//    return next ? next : Integer::getMinValue(width);
}

Integer::Ptr IntegerWidening::get_signed_next(const Integer::Ptr& value, DomainFactory*) const {
    if (not value->isValue()) return value;
    auto width = value->getWidth();
    return Integer::getMaxValue(width);
//
//    Integer::Ptr next;
//    auto mten = factory->toInteger(-10, width, true);
//    auto ten = factory->toInteger(10, width);
//    auto zero = factory->toInteger(0, width);
//    if (value->getValue() == llvm::APInt::getSignedMaxValue(width)) {
//        next = Integer::getMaxValue(width);
//    } else {
//        if (value->slt(mten)) {
//            next = value->sdiv(ten);
//        } else if (value->slt(zero)) {
//            next = value->mul(factory->toInteger(-1, width, true));
//        } else {
//            next = value->mul(ten);
//        }
//    }
//    return next ? next : Integer::getMaxValue(width);
}

llvm::APFloat FloatWidening::get_next(const llvm::APFloat& value, DomainFactory*) const {
    auto& semantics = value.getSemantics();
    return util::getMaxValue(semantics);
//
//    auto opres = llvm::APFloat::opOK;
//    llvm::APFloat next = value;
//    auto zero = llvm::APFloat(semantics, "0");
//    auto ten = llvm::APFloat(semantics, "10");
//    auto mten = llvm::APFloat(semantics, "-10");
//    if (util::eq(value, util::getMaxValue(semantics))) {
//        next = util::getMaxValue(semantics);
//    } else {
//        if (util::lt(value, mten)) {
//            opres = next.divide(ten, getRoundingMode());
//        } else if (util::lt(value, zero)) {
//            opres = next.multiply(llvm::APFloat(semantics, "-1"), getRoundingMode());
//        } else {
//            opres = next.multiply(ten, getRoundingMode());
//        }
//    }
//    return (opres == llvm::APFloat::opOK) ? next : util::getMaxValue(semantics);
}

llvm::APFloat FloatWidening::get_prev(const llvm::APFloat& value, DomainFactory*) const {
    auto& semantics = value.getSemantics();
    return util::getMinValue(semantics);
//
//    auto opres = llvm::APFloat::opOK;
//    llvm::APFloat next = value;
//    auto zero = llvm::APFloat(semantics, "0");
//    auto ten = llvm::APFloat(semantics, "10");
//    if (util::eq(value, util::getMinValue(semantics))) {
//        next = util::getMinValue(semantics);
//    } else {
//        if (util::gt(value, ten)) {
//            opres = next.divide(ten, getRoundingMode());
//        } else if (util::gt(value, zero)) {
//            opres = next.multiply(llvm::APFloat(semantics, "-1"), getRoundingMode());
//        } else {
//            opres = next.multiply(ten, getRoundingMode());
//        }
//    }
//    return (opres == llvm::APFloat::opOK) ? next : util::getMinValue(semantics);
}

}   /* namespace absint */
}   /* namespace borealis */


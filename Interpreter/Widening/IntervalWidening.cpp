//
// Created by abdullin on 6/26/17.
//

#include "Interpreter/Domain/DomainFactory.h"
#include "Interpreter/Util.hpp"
#include "IntervalWidening.h"

namespace borealis {
namespace absint {
/// Integer
IntegerWidening::IntegerWidening(DomainFactory* factory) : WideningInterface(factory) {}

Integer::Ptr IntegerWidening::get_prev(const Integer::Ptr& value) {
    auto width = value->getWidth();

    Integer::Ptr next;
    auto ten = factory_->toInteger(10, width);
    if (value->getValue() == 0) {
        next = Integer::getMinValue(width);
    } else {
        auto temp = value->udiv(ten);
        if (not temp) {
            next = Integer::getMinValue(width);
        } else {
            next = temp;
        }
    }
    return next;
}

Integer::Ptr IntegerWidening::get_next(const Integer::Ptr& value) {
    auto width = value->getWidth();

    Integer::Ptr next;
    auto ten = factory_->toInteger(10, width);
    if (value->getValue() == 0) {
        next = ten;
    } else {
        auto temp = value->mul(ten);
        if (not temp) {
            next = Integer::getMaxValue(width);
        } else {
            if (temp->gt(factory_->toInteger(-1, width))) {
                next = Integer::getMaxValue(width);
            } else {
                next = temp;
            }
        }
    }
    return next;
}


/// Float
FloatWidening::FloatWidening(DomainFactory* factory) : WideningInterface(factory) {}

llvm::APFloat FloatWidening::get_next(const llvm::APFloat& value) {
    auto& semantics = value.getSemantics();
    return util::getMaxValue(semantics);
}

llvm::APFloat FloatWidening::get_prev(const llvm::APFloat& value) {
    auto& semantics = value.getSemantics();
    return util::getMinValue(semantics);
}

}   /* namespace absint */
}   /* namespace borealis */


//
// Created by abdullin on 6/26/17.
//

#include "Interpreter/Domain/DomainFactory.h"
#include "Interpreter/Util.h"
#include "IntervalWidening.h"

namespace borealis {
namespace absint {

IntegerWidening::IntegerWidening(DomainFactory* factory) : WideningInterface(factory) {}

Integer::Ptr IntegerWidening::get_prev(const Integer::Ptr& value) {
    auto width = value->getWidth();
    Integer::Ptr next;
    auto ten = factory_->toInteger(10, width);
    if (value->getValue() == 0) {
        next = util::getMinValue(width);
    } else {
        auto temp = value->udiv(ten);
        if (not temp) {
            next = util::getMinValue(width);
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
            next = util::getMaxValue(width);
        } else {
            if (temp->gt(factory_->toInteger(-1, width))) {
                next = util::getMaxValue(width);
            } else {
                next = temp;
            }
        }
    }
    return next;
}

}   /* namespace absint */
}   /* namespace borealis */


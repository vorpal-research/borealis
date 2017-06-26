//
// Created by abdullin on 6/26/17.
//

#ifndef BOREALIS_INTERVALWIDENING_H
#define BOREALIS_INTERVALWIDENING_H

#include "Interpreter/Domain/Integer/Integer.h"
#include "WideningInterface.hpp"

namespace borealis {
namespace absint {

class IntegerWidening : public WideningInterface<Integer::Ptr> {
public:

    IntegerWidening(DomainFactory* factory);

    virtual Integer::Ptr get_prev(const Integer::Ptr& value);
    virtual Integer::Ptr get_next(const Integer::Ptr& value);

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERVALWIDENING_H

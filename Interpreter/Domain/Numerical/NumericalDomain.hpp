//
// Created by abdullin on 1/28/19.
//

#ifndef BOREALIS_NUMERICALDOMAIN_HPP
#define BOREALIS_NUMERICALDOMAIN_HPP

#include "Interpreter/Domain/Domain.h"
#include "Number.hpp"
#include "Interval.hpp"

namespace borealis {
namespace absint {

template <typename Number, typename Variable>
class NumericalDomain : public AbstractDomain {
public:

    virtual Ptr toInterval(Variable x) const = 0;

    virtual void assign(Variable x, int n) = 0;
    virtual void assign(Variable x, Number& n) = 0;
    virtual void assign(Variable x, Variable y) = 0;
    virtual void assign(Variable x, Ptr& i) = 0;

    virtual void apply(BinaryOperator op, Variable x, Variable y, Variable z) = 0;
    virtual Ptr apply(CmpOperator op, Variable x, Variable y) = 0;
};

} // namespace absint
} // namespace borealis

#endif //BOREALIS_NUMERICALDOMAIN_HPP

//
// Created by abdullin on 2/26/19.
//

#ifndef BOREALIS_AGGREGATE_HPP
#define BOREALIS_AGGREGATE_HPP

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"

namespace borealis {
namespace absint {

template <typename Variable>
class Aggregate : public AbstractDomain {
public:
    explicit Aggregate(id_t id) : AbstractDomain(id) {}

    virtual Ptr get(Variable x) const = 0;

    virtual void assign(Variable x, Ptr i) = 0;

    virtual Ptr extractFrom(Type::Ptr type, Variable ptr, Ptr index) const = 0;
    virtual void insertTo(Variable ptr, Ptr value, Ptr index) = 0;
};

} // namespace absint
} // namespace borealis

#endif //BOREALIS_AGGREGATE_HPP

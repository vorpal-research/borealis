//
// Created by abdullin on 2/13/19.
//

#ifndef BOREALIS_MEMORYDOMAIN_HPP
#define BOREALIS_MEMORYDOMAIN_HPP

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"

namespace borealis {
namespace absint {

template <typename MachineInt, typename Variable>
class MemoryDomain : public AbstractDomain {
public:
    explicit MemoryDomain(id_t id) : AbstractDomain(id) {}

    virtual Ptr get(Variable x) const = 0;

    virtual void assign(Variable x, Ptr i) = 0;
    virtual Ptr applyTo(llvm::ConditionType op, Variable x, Variable y) const = 0;

    virtual void addConstraint(llvm::ConditionType op, Variable x, Variable y) = 0;

    virtual Ptr loadFrom(Type::Ptr, Variable ptr) const = 0;
    virtual void storeTo(Variable ptr, Ptr x) = 0;
    virtual void gepFrom(Variable x, Type::Ptr, Variable ptr, const std::vector<Ptr>& shifts) = 0;
};

} // namespace absint
} // namespace borealis

#endif //BOREALIS_MEMORYDOMAIN_HPP

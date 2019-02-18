//
// Created by abdullin on 2/12/19.
//

#ifndef BOREALIS_DOMAINSTORAGE_HPP
#define BOREALIS_DOMAINSTORAGE_HPP

#include "AbstractDomain.hpp"
#include "AbstractFactory.hpp"
#include "Numerical/Number.hpp"

namespace borealis {
namespace absint {

class VariableFactory;

template <typename Variable>
class NumericalDomain;

template <typename MachineInt, typename Variable>
class MemoryDomain;

class DomainStorage : public logging::ObjectLevelLogging<DomainStorage>, public std::enable_shared_from_this<DomainStorage> {
public:

    using Ptr = std::shared_ptr<DomainStorage>;
    using Variable = const llvm::Value*;

private:

    using SIntT = AbstractFactory::SInt;
    using UIntT = AbstractFactory::UInt;
    using MachineIntT = AbstractFactory::MachineInt;

    using NumericalDomainT = NumericalDomain<Variable>;
    using MemoryDomainT = MemoryDomain<MachineIntT, Variable>;

protected:

    NumericalDomainT* unwrapFloat() const;
    NumericalDomainT* unwrapInt() const;
    MemoryDomainT* unwrapMemory() const;

public:

    explicit DomainStorage(VariableFactory* vf);
    DomainStorage(const DomainStorage&) = default;
    DomainStorage(DomainStorage&&) = default;
    DomainStorage& operator=(const DomainStorage&) = default;
    DomainStorage& operator=(DomainStorage&&) = default;

    DomainStorage::Ptr clone() const;
    bool equals(DomainStorage::Ptr other) const;

    void joinWith(DomainStorage::Ptr other);
    DomainStorage::Ptr join(DomainStorage::Ptr other);

    bool empty() const;

    AbstractDomain::Ptr get(Variable x) const;
    void assign(Variable x, Variable y) const;
    void assign(Variable x, AbstractDomain::Ptr domain) const;

    /// x = y op z
    void apply(llvm::BinaryOperator::BinaryOps op, Variable x, Variable y, Variable z);

    /// x = y op z
    void apply(llvm::CmpInst::Predicate op, Variable x, Variable y, Variable z);

    /// x = cast(op, y)
    void apply(CastOperator op, Variable x, Variable y);

    /// x = *ptr
    void load(Variable x, Variable ptr);

    /// *ptr = x
    void store(Variable ptr, Variable x);

    /// x = gep(ptr, shifts)
    void gep(Variable x, Variable ptr, const std::vector<Variable>& shifts);

    /// x = allocate<decltype(x)>(size);
    void allocate(Variable x, Variable size);

private:

    VariableFactory* vf_;
    AbstractDomain::Ptr ints_;
    AbstractDomain::Ptr floats_;
    AbstractDomain::Ptr memory_;

};

} // namespace absint
} // namespace borealis

#endif //BOREALIS_DOMAINSTORAGE_HPP

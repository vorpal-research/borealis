//
// Created by abdullin on 2/12/19.
//

#ifndef BOREALIS_DOMAINSTORAGE_HPP
#define BOREALIS_DOMAINSTORAGE_HPP

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"
#include "Interpreter/Domain/Numerical/Number.hpp"
#include "State.h"
#include "Term/Term.h"
#include "Term/BinaryTerm.h"
#include "Term/CmpTerm.h"

namespace borealis {
namespace absint {

class VariableFactory;

template <typename Variable>
class NumericalDomain;

template <typename MachineInt, typename Variable>
class MemoryDomain;

template <typename Variable>
class Aggregate;

namespace ps {

class DomainStorage
        : public logging::ObjectLevelLogging<DomainStorage>, public std::enable_shared_from_this<DomainStorage> {
public:

    using Ptr = std::shared_ptr<DomainStorage>;
    using Variable = Term::Ptr;
    using SplitMap = std::unordered_map<Variable, Split, TermHash, TermEqualsWType>;

    using SIntT = AbstractFactory::SInt;
    using UIntT = AbstractFactory::UInt;
    using MachineIntT = AbstractFactory::MachineInt;

    using NumericalDomainT = NumericalDomain<Variable>;
    using MemoryDomainT = MemoryDomain<MachineIntT, Variable>;
    using AggregateDomainT = Aggregate<Variable>;

protected:

    NumericalDomainT* unwrapBool() const;
    NumericalDomainT* unwrapInt() const;
    NumericalDomainT* unwrapFloat() const;
    MemoryDomainT* unwrapMemory() const;
    AggregateDomainT* unwrapStruct() const;

public:

    DomainStorage(const VariableFactory* vf, DomainStorage::Ptr input = nullptr);
    DomainStorage(const DomainStorage&) = default;
    DomainStorage(DomainStorage&&) = default;
//    DomainStorage& operator=(const DomainStorage&) = default;
//    DomainStorage& operator=(DomainStorage&&) = default;

    DomainStorage::Ptr clone() const;
    bool equals(DomainStorage::Ptr other) const;

    void joinWith(DomainStorage::Ptr other);
    DomainStorage::Ptr join(DomainStorage::Ptr other);

    bool empty() const;

    AbstractDomain::Ptr get(Variable x) const;
    void assign(Variable x, Variable y) const;
    void assign(Variable x, AbstractDomain::Ptr domain) const;

    /// x = y op z
    void apply(llvm::ArithType op, Variable x, Variable y, Variable z);

    /// x = y op z
    void apply(llvm::ConditionType op, Variable x, Variable y, Variable z);

    /// x = cast(op, y)
    void apply(CastOperator op, Variable x, Variable y);

    /// x = *ptr
    void load(Variable x, Variable ptr);

    /// *ptr = x
    void store(Variable ptr, Variable x);

    /// *ptr = (x->widen(*ptr))
    void storeWithWidening(Variable ptr, Variable x);

    /// x = gep(ptr, shifts)
    void gep(Variable x, Variable ptr, const std::vector<Variable>& shifts);

    /// x = extract(struct, index)
    void extract(Variable x, Variable structure, Variable index);

    /// insert(struct, x, index)
    void insert(Variable structure, Variable x, Variable index);

    /// x = allocate<decltype(x)>(size);
    void allocate(Variable x, Variable size);

    size_t hashCode() const;
    std::string toString() const;

private:

    const VariableFactory* vf_;
    const Ptr input_;
    AbstractDomain::Ptr bools_;
    AbstractDomain::Ptr ints_;
    AbstractDomain::Ptr floats_;
    AbstractDomain::Ptr memory_;
    AbstractDomain::Ptr structs_;

};

} // namespace ir
} // namespace absint
} // namespace borealis

#endif //BOREALIS_DOMAINSTORAGE_HPP

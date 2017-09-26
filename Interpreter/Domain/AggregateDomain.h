//
// Created by abdullin on 4/7/17.
//

#ifndef BOREALIS_ARRAY_H
#define BOREALIS_ARRAY_H

#include <unordered_map>

#include "Interpreter/Domain/Domain.h"

namespace borealis {
namespace absint {

/// Mutable
class AggregateDomain : public Domain {
public:

    using Types = std::vector<const llvm::Type*>;
    using Elements = std::unordered_map<std::size_t, Domain::Ptr>;

public:

    enum AggregateType {
        ARRAY = 0,
        STRUCT
    };

    /// Struct constructors
    AggregateDomain(Domain::Value value, DomainFactory* factory,
                    const AggregateDomain::Types& elementTypes, Domain::Ptr length);
    AggregateDomain(DomainFactory* factory, const AggregateDomain::Types& elementTypes, Domain::Ptr length);
    AggregateDomain(DomainFactory* factory, const AggregateDomain::Types& elementTypes,
                    const AggregateDomain::Elements& elements);
    /// Array constructors
    AggregateDomain(Domain::Value value, DomainFactory* factory,
                    const llvm::Type& elementType, Domain::Ptr length);
    AggregateDomain(DomainFactory* factory, const llvm::Type& elementType, Domain::Ptr length);
    AggregateDomain(DomainFactory* factory, const llvm::Type& elementType,
                    const AggregateDomain::Elements& elements);

    AggregateDomain(const AggregateDomain& other);

    void moveToTop() override;
    /// Poset
    bool equals(const Domain* other) const override;
    bool operator<(const Domain& other) const override;

    /// Lattice
    Domain::Ptr join(Domain::Ptr other) override;
    Domain::Ptr meet(Domain::Ptr other) override;
    Domain::Ptr widen(Domain::Ptr other) override;

    /// Other
    Domain::Ptr clone() const override;
    std::size_t hashCode() const override;
    std::string toPrettyString(const std::string& prefix) const override;
    const Types& getElementTypes() const;
    const Elements& getElements() const;
    Domain::Ptr getLength() const;
    std::size_t getMaxLength() const;
    bool isMaxLengthTop() const;
    bool isArray() const;
    bool isStruct() const;

    /// Aggregate
    Domain::Ptr extractValue(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) override;
    void insertValue(Domain::Ptr element, const std::vector<Domain::Ptr>& indices) override;
    /// Memory
    void store(Domain::Ptr value, Domain::Ptr offset) override;
    Domain::Ptr load(const llvm::Type& type, Domain::Ptr offset) override;
    Domain::Ptr gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) override;

    static bool classof(const Domain* other);

private:

    const llvm::Type& getElementType(std::size_t index) const;

    const AggregateType aggregateType_;
    const Types elementTypes_;
    Domain::Ptr length_;
    Elements elements_;

};

}   /* namespace absint */
}   /* namespace borealis */


#endif //BOREALIS_ARRAY_H

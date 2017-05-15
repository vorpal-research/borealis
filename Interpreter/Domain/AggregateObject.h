//
// Created by abdullin on 4/7/17.
//

#ifndef BOREALIS_ARRAY_H
#define BOREALIS_ARRAY_H

#include <unordered_map>

#include "Domain.h"
#include "MemoryObject.h"

namespace borealis {
namespace absint {

/// Mutable
class AggregateObject : public Domain {
public:

    using Types = std::unordered_map<std::size_t, const llvm::Type*>;
    using Elements = std::unordered_map<std::size_t, MemoryObject::Ptr>;

protected:

    enum AggregateType {
        ARRAY = 0,
        STRUCT
    };

    friend class DomainFactory;

    /// Struct constructors
    AggregateObject(Domain::Value value, DomainFactory* factory, const AggregateObject::Types& elementTypes, Domain::Ptr length);
    AggregateObject(DomainFactory* factory, const AggregateObject::Types& elementTypes, Domain::Ptr length);
    AggregateObject(DomainFactory* factory, const AggregateObject::Types& elementTypes, const AggregateObject::Elements& elements);
    /// Array constructors
    AggregateObject(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType, Domain::Ptr length);
    AggregateObject(DomainFactory* factory, const llvm::Type& elementType, Domain::Ptr length);
    AggregateObject(DomainFactory* factory, const llvm::Type& elementType, const AggregateObject::Elements& elements);
    AggregateObject(const AggregateObject& other);

public:
    /// Poset
    virtual bool equals(const Domain* other) const;
    virtual bool operator<(const Domain& other) const;

    /// Lattice
    virtual Domain::Ptr join(Domain::Ptr other) const;
    virtual Domain::Ptr meet(Domain::Ptr other) const;
    virtual Domain::Ptr widen(Domain::Ptr other) const;
    virtual Domain::Ptr narrow(Domain::Ptr other) const;

    /// Other
    virtual std::size_t hashCode() const;
    virtual std::string toString() const;
    virtual Domain* clone() const;
    const Types& getElementTypes() const;
    const Elements& getElements() const;
    Domain::Ptr getLength() const;
    std::size_t getMaxLength() const;
    bool isArray() const;
    bool isStruct() const;

    /// Aggregate
    virtual Domain::Ptr extractValue(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const;
    virtual void insertValue(Domain::Ptr element, const std::vector<Domain::Ptr>& indices) const;
    /// Memory
    virtual void store(Domain::Ptr value, Domain::Ptr offset) const;
    virtual Domain::Ptr load(const llvm::Type& type, Domain::Ptr offset) const;
    virtual Domain::Ptr gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const;

    static bool classof(const Domain* other);

private:

    const llvm::Type& getTypeFor(std::size_t index) const;

    AggregateType aggregateType_;
    Types elementTypes_;
    mutable Domain::Ptr length_;
    mutable Elements elements_;

};

}   /* namespace absint */
}   /* namespace borealis */


#endif //BOREALIS_ARRAY_H

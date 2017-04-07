//
// Created by abdullin on 4/7/17.
//

#ifndef BOREALIS_ARRAY_H
#define BOREALIS_ARRAY_H

#include "Domain.h"

namespace borealis {
namespace absint {

class Array : public Domain {
public:

    using Elements = std::vector<Domain::Ptr>;

protected:

    friend class DomainFactory;

    Array(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType);
    Array(DomainFactory* factory, const llvm::Type& elementType, Domain::Ptr length);
    Array(DomainFactory* factory, const llvm::Type& elementType, const Elements& elements);
    Array(const Array& other);

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
    virtual size_t hashCode() const;
    virtual std::string toString() const;
    virtual Domain* clone() const;
    const Elements& getElements() const;
    Domain::Ptr getLength() const;
    size_t getMaxLength() const;

    /// Aggregate
    virtual Domain::Ptr extractValue(const std::vector<Domain::Ptr>& indices) const;
    virtual void insertValue(Domain::Ptr element, const std::vector<Domain::Ptr>& indices) const;
    /// Memory
    virtual Domain::Ptr gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const;

    static bool classof(const Domain* other);

private:

    const llvm::Type& elementType_;
    mutable Elements elements_;
    mutable Domain::Ptr length_;

};

}   /* namespace absint */
}   /* namespace borealis */


#endif //BOREALIS_ARRAY_H

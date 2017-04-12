//
// Created by abdullin on 4/7/17.
//

#ifndef BOREALIS_POINTER_H
#define BOREALIS_POINTER_H

#include "Domain.h"

namespace borealis {
namespace absint {

class Pointer : public Domain {
public:

    using Locations = std::vector<Domain::Ptr>;

protected:

    friend class DomainFactory;

    Pointer(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType);
    Pointer(DomainFactory* factory, const llvm::Type& elementType, const Locations& locations);
    Pointer(const Pointer& other);
    virtual Domain& operator=(const Domain& other);

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
    const llvm::Type& getElementType() const;
    const Locations& getLocations() const;
    virtual size_t hashCode() const;
    virtual std::string toString() const;
    virtual Domain* clone() const;

    static bool classof(const Domain* other);

    /// Semantics
    virtual Domain::Ptr load(const llvm::Type& type, const std::vector<Domain::Ptr>& offsets) const;
    virtual void store(Domain::Ptr value, const std::vector<Domain::Ptr>& offsets) const;
    virtual Domain::Ptr gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const;
    /// Cast
    virtual Domain::Ptr ptrtoint(const llvm::Type& type) const;
    virtual Domain::Ptr bitcast(const llvm::Type& type) const;
    /// Cmp
    virtual Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;

private:

    const llvm::Type& elementType_;
    mutable Locations locations_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_POINTER_H

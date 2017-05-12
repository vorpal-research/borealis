//
// Created by abdullin on 4/7/17.
//

#ifndef BOREALIS_POINTER_H
#define BOREALIS_POINTER_H

#include <unordered_set>

#include "Domain.h"

namespace borealis {
namespace absint {

struct PointerLocation {
    mutable Domain::Ptr offset_;
    Domain::Ptr location_;
};

struct PtrLocationHash {
    size_t operator() (const PointerLocation& loc) const noexcept {
        return loc.location_->hashCode();
    }
};

struct PtrLocationEquals {
    bool operator() (const PointerLocation& lhv, const PointerLocation& rhv) const noexcept {
        return lhv.location_.get() == rhv.location_.get();
    }
};

/// Mutable
class Pointer : public Domain {
public:

    using Locations = std::unordered_set<PointerLocation, PtrLocationHash, PtrLocationEquals>;

protected:

    friend class DomainFactory;

    Pointer(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType);
    Pointer(DomainFactory* factory, const llvm::Type& elementType, const Locations& locations);
    Pointer(const Pointer& other);

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
    virtual std::size_t hashCode() const;
    virtual std::string toString() const;
    virtual Domain* clone() const;

    static bool classof(const Domain* other);

    /// Semantics
    virtual Domain::Ptr load(const llvm::Type& type, Domain::Ptr offset) const;
    virtual void store(Domain::Ptr value, Domain::Ptr offset) const;
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

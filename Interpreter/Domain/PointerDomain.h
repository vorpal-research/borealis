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
    mutable std::unordered_set<Domain::Ptr, DomainHash, DomainEquals> offsets_;
    Domain::Ptr location_;
};

struct PtrLocationHash {
    size_t operator() (const PointerLocation& loc) const noexcept {
        return loc.location_->hashCode();
    }
};

struct PtrLocationEquals {
    bool operator() (const PointerLocation& lhv, const PointerLocation& rhv) const noexcept {
        return lhv.location_->equals(rhv.location_.get());
//        return lhv.location_.get() == rhv.location_.get();
    }
};

class NullptrDomain : public Domain {
public:
    explicit NullptrDomain(DomainFactory* factory);

    void moveToTop() override {};
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

    /// Memory
    void store(Domain::Ptr value, Domain::Ptr offset) override;
    Domain::Ptr load(const llvm::Type& type, Domain::Ptr offset) override;
    Domain::Ptr gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) override;

    static bool classof(const Domain* other);
};

/// Mutable
class PointerDomain : public Domain {
public:

    using Locations = std::unordered_set<PointerLocation, PtrLocationHash, PtrLocationEquals>;

protected:

    friend class DomainFactory;

    PointerDomain(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType);
    PointerDomain(DomainFactory* factory, const llvm::Type& elementType, const Locations& locations);

public:
    PointerDomain(const PointerDomain& other);

    void moveToTop() override;
    /// Poset
    bool equals(const Domain* other) const override;
    bool operator<(const Domain& other) const override;

    /// Lattice
    Domain::Ptr join(Domain::Ptr other) override;
    Domain::Ptr meet(Domain::Ptr other) override;
    Domain::Ptr widen(Domain::Ptr other) override;

    /// Other
    const llvm::Type& getElementType() const;
    const Locations& getLocations() const;
    Domain::Ptr clone() const override;
    std::size_t hashCode() const override;
    std::string toPrettyString(const std::string& prefix) const override;

    static bool classof(const Domain* other);

    /// Semantics
    Domain::Ptr load(const llvm::Type& type, Domain::Ptr offset) override;
    void store(Domain::Ptr value, Domain::Ptr offset) override;
    Domain::Ptr gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) override;
    /// Cast
    Domain::Ptr ptrtoint(const llvm::Type& type) override;
    Domain::Ptr bitcast(const llvm::Type& type) override;
    /// Cmp
    Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const override;
    /// Split
    Split splitByEq(Domain::Ptr other) override;

private:

    const llvm::Type& elementType_;
    Locations locations_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_POINTER_H

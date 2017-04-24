//
// Created by abdullin on 4/14/17.
//

#ifndef BOREALIS_GEPPOINTER_H
#define BOREALIS_GEPPOINTER_H

#include <vector>
#include <unordered_set>

#include "Domain.h"
#include "MemoryObject.h"

namespace borealis {
namespace absint {

class GepPointer : public Domain {
public:

    using Objects = std::unordered_set<MemoryObject::Ptr, MemoryObjectHash, MemoryObjectEquals>;

protected:

    GepPointer(DomainFactory* factory, const llvm::Type& elementType, const Objects& objects);
    GepPointer(const GepPointer& other);

    friend class DomainFactory;

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
    const Objects& getObjects() const;
    virtual std::size_t hashCode() const;
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
    Objects objects_;
};

}
}

#endif //BOREALIS_GEPPOINTER_H

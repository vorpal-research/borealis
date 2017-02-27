//
// Created by abdullin on 2/17/17.
//

#ifndef BOREALIS_POINTER_H
#define BOREALIS_POINTER_H

#include "Domain.h"

namespace borealis {
namespace absint {

class Pointer : public Domain {
public:
    enum Status {VALID, NON_VALID};

protected:

    friend class DomainFactory;

    Pointer(const DomainFactory* factory);
    Pointer(Domain::Value value, const DomainFactory* factory);
    Pointer(const DomainFactory* factory, Pointer::Status status);
    Pointer(const Pointer& other);

public:
    /// Poset
    virtual bool equals(const Domain* other) const;
    virtual bool operator<(const Domain& other) const;

    /// Lattice
    virtual Domain::Ptr join(Domain::Ptr other) const;
    virtual Domain::Ptr meet(Domain::Ptr other) const;
    virtual Domain::Ptr widen(Domain::Ptr other) const;

    /// Other
    Pointer::Status getStatus() const;
    bool isValid() const;
    virtual size_t hashCode() const;
    virtual std::string toString() const;
    virtual Domain* clone() const;

    /// Semantics
    virtual Domain::Ptr load(const llvm::Type& type, const std::vector<Domain::Ptr>& offsets) const;
    virtual Domain::Ptr store(Domain::Ptr value, const std::vector<Domain::Ptr>& offsets) const;
    virtual Domain::Ptr gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const;
    /// Cast
    virtual Domain::Ptr ptrtoint(const llvm::Type& type) const;
    virtual Domain::Ptr bitcast(const llvm::Type& type) const;
    /// Cmp
    virtual Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;

private:

    Status status_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_POINTER_H

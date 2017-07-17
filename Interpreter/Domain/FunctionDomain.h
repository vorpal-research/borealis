//
// Created by abdullin on 7/14/17.
//

#ifndef BOREALIS_FUNCTIONDOMAIN_H
#define BOREALIS_FUNCTIONDOMAIN_H

#include <unordered_set>

#include "Domain.h"
#include "Interpreter/IR/Function.h"

namespace borealis {
namespace absint {

class FunctionDomain : public Domain {
public:

    using FunctionSet = std::unordered_set<Function::Ptr, FunctionHash, FunctionEquals>;

protected:

    friend class DomainFactory;
    FunctionDomain(DomainFactory* factory, const llvm::Type* type);
    FunctionDomain(DomainFactory* factory, const llvm::Type* type, Function::Ptr location);
    FunctionDomain(DomainFactory* factory, const llvm::Type* type, const FunctionSet& locations);

public:
    /// Poset
    virtual bool equals(const Domain* other) const;
    virtual bool operator<(const Domain& other) const;

    /// Lattice
    virtual Domain::Ptr join(Domain::Ptr other) const;
    virtual Domain::Ptr meet(Domain::Ptr other) const;
    virtual Domain::Ptr widen(Domain::Ptr other) const;

    /// Other
    const FunctionSet& getLocations() const;
    virtual std::size_t hashCode() const;
    virtual std::string toPrettyString(const std::string& prefix) const;

    static bool classof(const Domain* other);

private:

    const llvm::Type* type_;
    mutable FunctionSet locations_;

};

}   // namespace absint
}   // namespace borealis

#endif //BOREALIS_FUNCTIONDOMAIN_H

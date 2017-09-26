//
// Created by abdullin on 7/14/17.
//

#ifndef BOREALIS_FUNCTIONDOMAIN_H
#define BOREALIS_FUNCTIONDOMAIN_H

#include <unordered_set>

#include "Interpreter/Domain/Domain.h"
#include "Interpreter/IR/Function.h"

namespace borealis {
namespace absint {

class FunctionDomain : public Domain {
public:

    using FunctionSet = std::unordered_set<Function::Ptr, FunctionHash, FunctionEquals>;

    FunctionDomain(DomainFactory* factory, const llvm::Type* type);
    FunctionDomain(DomainFactory* factory, const llvm::Type* type, Function::Ptr location);
    FunctionDomain(DomainFactory* factory, const llvm::Type* type, const FunctionSet& locations);
    FunctionDomain(const FunctionDomain& other);

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
    const FunctionSet& getLocations() const;
    std::size_t hashCode() const override;
    std::string toPrettyString(const std::string& prefix) const override;

    static bool classof(const Domain* other);

private:

    const llvm::Type* prototype_;
    FunctionSet locations_;

};

}   // namespace absint
}   // namespace borealis

#endif //BOREALIS_FUNCTIONDOMAIN_H

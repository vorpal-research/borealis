//
// Created by abdullin on 2/2/17.
//

#ifndef BOREALIS_INTERVALDOMAIN_H
#define BOREALIS_INTERVALDOMAIN_H

#include "Interpreter/Domain/Domain.h"
#include "Interpreter/Domain/Integer/Integer.h"
#include "Interpreter/Domain/IntervalWidening.hpp"
#include "Interpreter/Util.hpp"
#include "Util/hash.hpp"

namespace borealis {
namespace absint {

class IntegerIntervalDomain : public Domain {
public:

    /// Structure that identifies int interval
    using ID = std::tuple<Domain::Value, Integer::Ptr, Integer::Ptr, Integer::Ptr, Integer::Ptr>;
    struct IDHash;
    struct IDEquals;

    IntegerIntervalDomain(DomainFactory* factory, Integer::Ptr constant);
    IntegerIntervalDomain(DomainFactory* factory, Integer::Ptr lb, Integer::Ptr ub);
    IntegerIntervalDomain(DomainFactory* factory, Integer::Ptr lb, Integer::Ptr ub, Integer::Ptr slb, Integer::Ptr sub);
    IntegerIntervalDomain(DomainFactory* factory, const ID& key);
    IntegerIntervalDomain(const IntegerIntervalDomain& other);

    /// Poset
    bool equals(const Domain* other) const override;
    bool operator<(const Domain& other) const override;

    /// Lattice
    Domain::Ptr join(Domain::Ptr other) override;
    Domain::Ptr meet(Domain::Ptr other) override;
    Domain::Ptr widen(Domain::Ptr other) override;

    /// Other
    size_t getWidth() const;
    bool isConstant() const;
    bool isConstant(uint64_t constant) const;
    Integer::Ptr lb() const;
    Integer::Ptr ub() const;
    Integer::Ptr signed_lb() const;
    Integer::Ptr signed_ub() const;
    bool hasIntersection(Integer::Ptr constant) const;
    bool hasSignedIntersection(Integer::Ptr constant) const;
    bool hasIntersection(const IntegerIntervalDomain* other) const;
    bool hasSignedIntersection(const IntegerIntervalDomain* other) const;

    Domain::Ptr clone() const override;
    size_t hashCode() const override;
    std::string toPrettyString(const std::string& prefix) const override;

    static bool classof(const Domain* other);

    /// Semantics
    Domain::Ptr add(Domain::Ptr other) const override;
    Domain::Ptr sub(Domain::Ptr other) const override;
    Domain::Ptr mul(Domain::Ptr other) const override;
    Domain::Ptr udiv(Domain::Ptr other) const override;
    Domain::Ptr sdiv(Domain::Ptr other) const override;
    Domain::Ptr urem(Domain::Ptr other) const override;
    Domain::Ptr srem(Domain::Ptr other) const override;
    /// Shifts
    Domain::Ptr shl(Domain::Ptr other) const override;
    Domain::Ptr lshr(Domain::Ptr other) const override;
    Domain::Ptr ashr(Domain::Ptr other) const override;
    /// Binary
    Domain::Ptr bAnd(Domain::Ptr other) const override;
    Domain::Ptr bOr(Domain::Ptr other) const override;
    Domain::Ptr bXor(Domain::Ptr other) const override;
    /// Cast
    Domain::Ptr trunc(const llvm::Type& type) const override;
    Domain::Ptr zext(const llvm::Type& type) const override;
    Domain::Ptr sext(const llvm::Type& type) const override;
    Domain::Ptr uitofp(const llvm::Type& type) const override;
    Domain::Ptr sitofp(const llvm::Type& type) const override;
    Domain::Ptr inttoptr(const llvm::Type& type) const override;
    Domain::Ptr bitcast(const llvm::Type& type) override;
    /// Other
    Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const override;
    /// Split operations, assume that intervals intersect
    Split splitByEq(Domain::Ptr other) override;
    Split splitByLess(Domain::Ptr other) override;
    Split splitBySLess(Domain::Ptr other) override;

private:

    const Integer::Ptr lb_;
    const Integer::Ptr ub_;
    const Integer::Ptr signed_lb_;
    const Integer::Ptr signed_ub_;
    const IntegerWidening* wm_;
};

struct IntegerIntervalDomain::IDHash {
    size_t operator()(const ID& id) const {
        return std::hash<ID>()(id);
    }
};

struct IntegerIntervalDomain::IDEquals {
    bool operator()(const ID& lhv, const ID& rhv) const {
        return std::get<0>(lhv) == std::get<0>(rhv) &&
               std::get<1>(lhv)->eq(std::get<1>(rhv)) &&
               std::get<2>(lhv)->eq(std::get<2>(rhv)) &&
               std::get<3>(lhv)->eq(std::get<3>(rhv)) &&
               std::get<4>(lhv)->eq(std::get<4>(rhv));
    }
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERVALDOMAIN_H

//
// Created by abdullin on 2/2/17.
//

#ifndef BOREALIS_INTERVALDOMAIN_H
#define BOREALIS_INTERVALDOMAIN_H

#include "Domain.h"
#include "Integer/Integer.h"
#include "Interpreter/Util.h"
#include "Interpreter/Widening/IntervalWidening.h"
#include "Util/hash.hpp"

namespace borealis {
namespace absint {

class IntegerInterval : public Domain {
public:

    /// Structure that identifies int interval
    using ID = std::tuple<Domain::Value, Integer::Ptr, Integer::Ptr>;
    struct IDHash;
    struct IDEquals;

protected:

    friend class DomainFactory;

    IntegerInterval(DomainFactory* factory, Integer::Ptr constant);
    IntegerInterval(DomainFactory* factory, Integer::Ptr from, Integer::Ptr to);
    IntegerInterval(DomainFactory* factory, const ID& key);

public:
    /// Poset
    virtual bool equals(const Domain* other) const;
    virtual bool operator<(const Domain& other) const;

    /// Lattice
    virtual Domain::Ptr join(Domain::Ptr other) const;
    virtual Domain::Ptr meet(Domain::Ptr other) const;
    virtual Domain::Ptr widen(Domain::Ptr other) const;

    /// Other
    size_t getWidth() const;
    bool isConstant() const;
    bool isConstant(uint64_t constant) const;
    Integer::Ptr from() const;
    Integer::Ptr to() const;
    Integer::Ptr signedFrom() const;
    Integer::Ptr signedTo() const;
    bool hasIntersection(Integer::Ptr constant) const;
    bool hasIntersection(const IntegerInterval* other) const;

    virtual size_t hashCode() const;
    virtual std::string toPrettyString(const std::string& prefix) const;

    static bool classof(const Domain* other);

    /// Semantics
    virtual Domain::Ptr add(Domain::Ptr other) const;
    virtual Domain::Ptr sub(Domain::Ptr other) const;
    virtual Domain::Ptr mul(Domain::Ptr other) const;
    virtual Domain::Ptr udiv(Domain::Ptr other) const;
    virtual Domain::Ptr sdiv(Domain::Ptr other) const;
    virtual Domain::Ptr urem(Domain::Ptr other) const;
    virtual Domain::Ptr srem(Domain::Ptr other) const;
    /// Shifts
    virtual Domain::Ptr shl(Domain::Ptr other) const;
    virtual Domain::Ptr lshr(Domain::Ptr other) const;
    virtual Domain::Ptr ashr(Domain::Ptr other) const;
    /// Binary
    virtual Domain::Ptr bAnd(Domain::Ptr other) const;
    virtual Domain::Ptr bOr(Domain::Ptr other) const;
    virtual Domain::Ptr bXor(Domain::Ptr other) const;
    /// Cast
    virtual Domain::Ptr trunc(const llvm::Type& type) const;
    virtual Domain::Ptr zext(const llvm::Type& type) const;
    virtual Domain::Ptr sext(const llvm::Type& type) const;
    virtual Domain::Ptr uitofp(const llvm::Type& type) const;
    virtual Domain::Ptr sitofp(const llvm::Type& type) const;
    virtual Domain::Ptr inttoptr(const llvm::Type& type) const;
    virtual Domain::Ptr bitcast(const llvm::Type& type) const;
    /// Other
    virtual Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;
    /// Split operations, assume that intervals intersect
    virtual Split splitByEq(Domain::Ptr other) const;
    virtual Split splitByLess(Domain::Ptr other) const;
    virtual Split splitBySLess(Domain::Ptr other) const;

private:

    virtual void setTop();

    Integer::Ptr from_;
    Integer::Ptr to_;
    mutable IntegerWidening wm_;
};

struct IntegerInterval::IDHash {
    size_t operator()(const ID& id) const {
        return std::hash<ID>()(id);
    }
};

struct IntegerInterval::IDEquals {
    bool operator()(const ID& lhv, const ID& rhv) const {
        return std::get<0>(lhv) == std::get<0>(rhv) &&
               std::get<1>(lhv)->eq(std::get<1>(rhv)) &&
               std::get<2>(lhv)->eq(std::get<2>(rhv));
    }
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERVALDOMAIN_H

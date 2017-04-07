//
// Created by abdullin on 2/2/17.
//

#ifndef BOREALIS_INTERVALDOMAIN_H
#define BOREALIS_INTERVALDOMAIN_H

#include "Domain.h"
#include "Util.h"
#include "Util/hash.hpp"

namespace borealis {
namespace absint {

class IntegerInterval : public Domain {
public:

    /// Structure that identifies int interval
    using ID = std::tuple<Domain::Value, bool, llvm::APInt, llvm::APInt>;
    struct IDHash;
    struct IDEquals;

protected:

    friend class DomainFactory;

    IntegerInterval(Domain::Value value, DomainFactory* factory, unsigned width, bool isSigned  = false);
    IntegerInterval(DomainFactory* factory, const llvm::APInt& constant, bool isSigned  = false);
    IntegerInterval(DomainFactory* factory, const llvm::APInt& from, const llvm::APInt& to, bool isSigned  = false);
    IntegerInterval(DomainFactory* factory, const ID& key);
    IntegerInterval(const IntegerInterval& interval);

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
    unsigned getWidth() const;
    bool isConstant() const;
    bool isConstant(uint64_t constant) const;
    const llvm::APInt& from() const;
    const llvm::APInt& to() const;
    bool intersects(const llvm::APInt& constant) const;
    bool intersects(const IntegerInterval* other) const;

    virtual size_t hashCode() const;
    virtual std::string toString() const;
    virtual Domain* clone() const;
    // changes sign of the domain if it's incorrect
    virtual bool isCorrect();

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

private:

    virtual void setTop();

    bool signed_;
    llvm::APInt from_;
    llvm::APInt to_;
};

struct IntegerInterval::IDHash {
    size_t operator()(const ID& id) const {
        return std::hash<ID>()(id);
    }
};

struct IntegerInterval::IDEquals {
    bool operator()(const ID& lhv, const ID& rhv) const {
        return std::get<0>(lhv) == std::get<0>(rhv) &&
               std::get<1>(lhv) == std::get<1>(rhv) &&
               util::eq(std::get<2>(lhv), std::get<2>(rhv)) &&
               util::eq(std::get<3>(lhv), std::get<3>(rhv));
    }
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERVALDOMAIN_H

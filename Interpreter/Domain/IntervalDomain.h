//
// Created by abdullin on 2/2/17.
//

#ifndef BOREALIS_INTERVALDOMAIN_H
#define BOREALIS_INTERVALDOMAIN_H

#include <Util/util.h>
#include "Domain.h"

namespace borealis {
namespace absint {

class IntervalDomain : public Domain {
public:

    friend class DomainFactory;

    IntervalDomain(unsigned width, bool isSigned = false);
    IntervalDomain(const llvm::APSInt& constant);
    IntervalDomain(const llvm::APSInt& from, const llvm::APSInt& to);
    IntervalDomain(const IntervalDomain& interval);

public:
    /// Poset
    virtual bool equals(const Domain* other) const;
    virtual bool operator<(const Domain& other) const;

    /// Lattice
    virtual Domain::Ptr join(Domain::Ptr other) const;
    virtual Domain::Ptr meet(Domain::Ptr other) const;

    /// Other
    bool equalFormat(const IntervalDomain& other) const;

    unsigned getWidth() const;
    bool isSigned() const;
    bool isConstant() const;
    const llvm::APSInt& from() const;
    const llvm::APSInt& to() const;
    bool intersects(const IntervalDomain* other) const;

    virtual size_t hashCode() const;
    virtual std::string toString() const;
    virtual Domain* clone() const;
    virtual bool isCorrect() const;

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
    /// Other
    virtual Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;

private:

    virtual void setTop();


    unsigned width_;
    llvm::APSInt from_;
    llvm::APSInt to_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERVALDOMAIN_H

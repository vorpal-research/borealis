//
// Created by abdullin on 2/2/17.
//

#ifndef BOREALIS_INTERVALDOMAIN_H
#define BOREALIS_INTERVALDOMAIN_H

#include <llvm/ADT/APSInt.h>

#include "Domain.h"

namespace borealis {
namespace absint {

class IntegerInterval : public Domain {
protected:

    friend class DomainFactory;

    IntegerInterval(const DomainFactory* factory, unsigned width, bool isSigned = false);
    IntegerInterval(Domain::Value value, const DomainFactory* factory, unsigned width, bool isSigned = false);
    IntegerInterval(const DomainFactory* factory, const llvm::APSInt& constant);
    IntegerInterval(const DomainFactory* factory, const llvm::APSInt& from, const llvm::APSInt& to);
    IntegerInterval(const IntegerInterval& interval);

public:
    /// Poset
    virtual bool equals(const Domain* other) const;
    virtual bool operator<(const Domain& other) const;

    /// Lattice
    virtual Domain::Ptr join(Domain::Ptr other) const;
    virtual Domain::Ptr meet(Domain::Ptr other) const;
    virtual Domain::Ptr widen(Domain::Ptr other) const;

    /// Other
    unsigned getWidth() const;
    bool isSigned() const;
    bool isConstant() const;
    const llvm::APSInt& from() const;
    const llvm::APSInt& to() const;
    bool intersects(const IntegerInterval* other) const;

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

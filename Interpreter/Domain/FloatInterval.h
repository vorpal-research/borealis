//
// Created by abdullin on 2/17/17.
//

#ifndef BOREALIS_FLOATINTERVAL_H
#define BOREALIS_FLOATINTERVAL_H

#include "Domain.h"

namespace borealis {
namespace absint {

class FloatInterval : public Domain {
protected:

    friend class DomainFactory;

    FloatInterval(const DomainFactory* factory, const llvm::fltSemantics& semantics);
    FloatInterval(Domain::Value value, const DomainFactory* factory, const llvm::fltSemantics& semantics);
    FloatInterval(const DomainFactory* factory, const llvm::APFloat& constant);
    FloatInterval(const DomainFactory* factory, const llvm::APFloat& from, const llvm::APFloat& to);
    FloatInterval(const FloatInterval& interval);

public:
    /// Poset
    virtual bool equals(const Domain* other) const;
    virtual bool operator<(const Domain& other) const;

    /// Lattice
    virtual Domain::Ptr join(Domain::Ptr other) const;
    virtual Domain::Ptr meet(Domain::Ptr other) const;
    virtual Domain::Ptr widen(Domain::Ptr other) const;

    const llvm::fltSemantics& getSemantics() const;
    bool isConstant() const;
    const llvm::APFloat& from() const;
    const llvm::APFloat& to() const;
    bool intersects(const FloatInterval* other) const;

    virtual size_t hashCode() const;
    virtual std::string toString() const;
    virtual Domain* clone() const;

    static bool classof(const Domain* other);

    /// Semantics
    virtual Domain::Ptr fadd(Domain::Ptr other) const;
    virtual Domain::Ptr fsub(Domain::Ptr other) const;
    virtual Domain::Ptr fmul(Domain::Ptr other) const;
    virtual Domain::Ptr fdiv(Domain::Ptr other) const;
    virtual Domain::Ptr frem(Domain::Ptr other) const;
    /// Cast
    virtual Domain::Ptr fptrunc(const llvm::Type& type) const;
    virtual Domain::Ptr fpext(const llvm::Type& type) const;
    virtual Domain::Ptr fptoui(const llvm::Type& type) const;
    virtual Domain::Ptr fptosi(const llvm::Type& type) const;
    /// Other
    virtual Domain::Ptr fcmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;

private:

    virtual void setTop();

    llvm::APFloat from_;
    llvm::APFloat to_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_FLOATINTERVAL_H

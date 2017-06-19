//
// Created by abdullin on 2/17/17.
//

#ifndef BOREALIS_FLOATINTERVAL_H
#define BOREALIS_FLOATINTERVAL_H

#include "Domain.h"
#include "Interpreter/Util.h"
#include "Util/hash.hpp"

namespace borealis {
namespace absint {

class FloatInterval : public Domain {
public:

    /// Structure that identifies float interval
    using ID = std::tuple<Domain::Value, llvm::APFloat, llvm::APFloat>;
    struct IDHash;
    struct IDEquals;

protected:

    friend class DomainFactory;

    FloatInterval(Domain::Value value, DomainFactory* factory, const llvm::fltSemantics& semantics);
    FloatInterval(DomainFactory* factory, const llvm::APFloat& constant);
    FloatInterval(DomainFactory* factory, const llvm::APFloat& from, const llvm::APFloat& to);
    FloatInterval(DomainFactory* factory, const ID& id);

public:
    /// Poset
    virtual bool equals(const Domain* other) const;
    virtual bool operator<(const Domain& other) const;

    /// Lattice
    virtual Domain::Ptr join(Domain::Ptr other) const;
    virtual Domain::Ptr meet(Domain::Ptr other) const;
    virtual Domain::Ptr widen(Domain::Ptr other) const;
    virtual Domain::Ptr narrow(Domain::Ptr other) const;

    const llvm::fltSemantics& getSemantics() const;
    llvm::APFloat::roundingMode getRoundingMode() const;
    bool isConstant() const;
    bool isNaN() const;
    const llvm::APFloat& from() const;
    const llvm::APFloat& to() const;
    bool hasIntersection(const FloatInterval* other) const;

    virtual size_t hashCode() const;
    virtual std::string toPrettyString(const std::string& prefix) const;

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
    virtual Domain::Ptr bitcast(const llvm::Type& type) const;
    /// Other
    virtual Domain::Ptr fcmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;
    /// Split operations
    virtual Split splitByEq(Domain::Ptr other) const;
    virtual Split splitByNeq(Domain::Ptr other) const;
    virtual Split splitByLess(Domain::Ptr other) const;

private:

    virtual void setTop();

    llvm::APFloat from_;
    llvm::APFloat to_;
};

struct FloatInterval::IDHash {
    size_t operator()(const ID& id) const {
        return std::hash<ID>()(id);
    }
};

struct FloatInterval::IDEquals {
    bool operator()(const ID& lhv, const ID& rhv) const {
        return std::get<0>(lhv) == std::get<0>(rhv) &&
               util::eq(std::get<1>(lhv), std::get<1>(rhv)) &&
               util::eq(std::get<2>(lhv), std::get<2>(rhv));
    }
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_FLOATINTERVAL_H

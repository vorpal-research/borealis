//
// Created by abdullin on 2/1/17.
//

#ifndef BOREALIS_DOMAIN_HPP
#define BOREALIS_DOMAIN_HPP

#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/Type.h>
#include <llvm/IR/InstrTypes.h>

#include "Logging/logger.hpp"

namespace borealis {
namespace absint {

class Domain : public std::enable_shared_from_this<const Domain>, public logging::ObjectLevelLogging<Domain> {
public:

    enum Type {IntegerInterval,
        FloatInterval};

    enum Value {TOP, VALUE, BOTTOM};

    using Ptr = std::shared_ptr<const Domain>;

    /// Poset
    virtual bool equals(const Domain *other) const = 0;

    virtual bool operator<(const Domain &other) const = 0;

    virtual bool operator==(const Domain &other) const {
        if (this == &other) return true;
        else return this->equals(&other);
    }

    virtual bool operator!=(const Domain &other) const {
        if (this == &other) return false;
        else return not this->equals(&other);
    }

    virtual bool operator<=(const Domain &other) const {
        return ((*this) < other) || ((*this) == other);
    }

    /// Lattice
protected:
    virtual void setTop() {
        value_ = TOP;
    }

    virtual void setBottom() {
        value_ = BOTTOM;
    }

    virtual void setValue() {
        value_ = VALUE;
    }

public:
    virtual bool isTop() const {
        return value_ == TOP;
    }

    virtual bool isBottom() const {
        return value_ == BOTTOM;
    }

    virtual bool isValue() const {
        return value_ == VALUE;
    }

    virtual Domain::Ptr join(Domain::Ptr other) const = 0;

    virtual Domain::Ptr meet(Domain::Ptr other) const = 0;

    /// Other
    virtual size_t hashCode() const = 0;
    virtual std::string toString() const = 0;
    virtual Domain* clone() const = 0;

    virtual bool isCorrect() const {
        return true;
    }

    virtual Type getType() const {
        return type_;
    }

    static bool classof(const Domain*) {
        return true;
    }

    /// LLVM Semantics
    /// Arithmetical
    virtual Domain::Ptr add(Domain::Ptr other) const;
    virtual Domain::Ptr fadd(Domain::Ptr other) const;
    virtual Domain::Ptr sub(Domain::Ptr other) const;
    virtual Domain::Ptr fsub(Domain::Ptr other) const;
    virtual Domain::Ptr mul(Domain::Ptr other) const;
    virtual Domain::Ptr fmul(Domain::Ptr other) const;
    virtual Domain::Ptr udiv(Domain::Ptr other) const;
    virtual Domain::Ptr sdiv(Domain::Ptr other) const;
    virtual Domain::Ptr fdiv(Domain::Ptr other) const;
    virtual Domain::Ptr urem(Domain::Ptr other) const;
    virtual Domain::Ptr srem(Domain::Ptr other) const;
    virtual Domain::Ptr frem(Domain::Ptr other) const;
    /// Shifts
    virtual Domain::Ptr shl(Domain::Ptr other) const;
    virtual Domain::Ptr lshr(Domain::Ptr other) const;
    virtual Domain::Ptr ashr(Domain::Ptr other) const;
    /// Binary
    virtual Domain::Ptr bAnd(Domain::Ptr other) const;
    virtual Domain::Ptr bOr(Domain::Ptr other) const;
    virtual Domain::Ptr bXor(Domain::Ptr other) const;
    /// Vector
    virtual Domain::Ptr extractElement(Domain::Ptr indx) const;
    virtual Domain::Ptr insertElement(Domain::Ptr element, Domain::Ptr indx) const;
    /// Aggregate
    virtual Domain::Ptr extractValue(const std::vector<Domain::Ptr>& indices) const;
    virtual Domain::Ptr insertValue(Domain::Ptr element, const std::vector<Domain::Ptr>& indices) const;
    /// Memory
    virtual Domain::Ptr load(const llvm::Type& type, const std::vector<Domain::Ptr>& offsets) const;
    virtual Domain::Ptr store(Domain::Ptr value, const std::vector<Domain::Ptr>& offsets) const;
    virtual Domain::Ptr gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const;
    /// Cast
    virtual Domain::Ptr trunc(const llvm::Type& type) const;
    virtual Domain::Ptr zext(const llvm::Type& type) const;
    virtual Domain::Ptr sext(const llvm::Type& type) const;
    virtual Domain::Ptr fptrunc(const llvm::Type& type) const;
    virtual Domain::Ptr fpext(const llvm::Type& type) const;
    virtual Domain::Ptr fptoui(const llvm::Type& type) const;
    virtual Domain::Ptr fptosi(const llvm::Type& type) const;
    virtual Domain::Ptr uitofp(const llvm::Type& type) const;
    virtual Domain::Ptr sitofp(const llvm::Type& type) const;
    virtual Domain::Ptr ptrtoint(const llvm::Type& type) const;
    virtual Domain::Ptr inttoptr(const llvm::Type& type) const;
    /// Other
    virtual Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;
    virtual Domain::Ptr fcmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;

protected:

    Domain(Domain::Value value, Domain::Type type) : ObjectLevelLogging("domain"), value_(value), type_(type) {}

    Value value_;
    Type type_;
};

}   /* namespace absint */
}   /* namespace borealis*/

#endif //BOREALIS_DOMAIN_HPP

//
// Created by abdullin on 2/1/17.
//

#ifndef BOREALIS_DOMAIN_HPP
#define BOREALIS_DOMAIN_HPP

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Type.h>

#include "Interpreter/Widening/WideningInterface.hpp"
#include "Logging/logger.hpp"

namespace borealis {
namespace absint {

struct Split;
class DomainFactory;
class MemoryObject;

class Domain : public std::enable_shared_from_this<const Domain>, public logging::ObjectLevelLogging<Domain> {
public:

    enum Type {INTEGER_INTERVAL = 0,
        FLOAT_INTERVAL,
        NULLPTR,
        POINTER,
        FUNCTION,
        AGGREGATE
    };

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

    /// change all mutable domains value to TOP
    virtual void moveToTop() const;

    virtual Domain::Ptr join(Domain::Ptr other) const = 0;
    virtual Domain::Ptr meet(Domain::Ptr other) const = 0;
    virtual Domain::Ptr widen(Domain::Ptr other) const = 0;

    /// Other
    virtual size_t hashCode() const = 0;

    virtual std::string toPrettyString(const std::string& prefix) const {
        return prefix + "unknown";
    }
    virtual std::string toString() const {
        return toPrettyString("");
    }

    virtual Type getType() const {
        return type_;
    }

    static bool classof(const Domain*) {
        return true;
    }

    /// type_ = AGGREGATE
    virtual bool isAggregate() const;
    /// type_ = POINTER || NULLPTR
    virtual bool isPointer() const;
    /// type_ = INTEGER || FLOAT
    virtual bool isSimple() const;
    /// type_ = INTEGER
    virtual bool isInteger() const;
    /// type_ = FLOAT
    virtual bool isFloat() const;
    /// type_ = FUNCTION
    virtual bool isFunction() const;
    /// type_ = NULLPTR
    virtual bool isNullptr() const;
    /// type_ = POINTER || AGGREGATE || FUNCTION
    virtual bool isMutable() const;

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
    virtual Domain::Ptr extractElement(const std::vector<Domain::Ptr>& indices) const;
    virtual void insertElement(Domain::Ptr element, const std::vector<Domain::Ptr>& indices) const;
    /// Aggregate
    virtual Domain::Ptr extractValue(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const;
    virtual void insertValue(Domain::Ptr element, const std::vector<Domain::Ptr>& indices) const;
    /// Memory
    /// @arg type - type of the result element
    virtual Domain::Ptr load(const llvm::Type& type, Domain::Ptr offset) const;
    virtual void store(Domain::Ptr value, Domain::Ptr offset) const;
    /// @arg type - type of the element that we want to get pointer to
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
    virtual Domain::Ptr bitcast(const llvm::Type& type) const;
    /// Other
    virtual Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;
    virtual Domain::Ptr fcmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;
    /// Split operations
    virtual Split splitByEq(Domain::Ptr other) const;
    virtual Split splitByLess(Domain::Ptr other) const;
    virtual Split splitBySLess(Domain::Ptr other) const;

protected:

    Domain(Domain::Value value, Domain::Type type, DomainFactory* factory)
            : ObjectLevelLogging("domain"),
              value_(value),
              type_(type),
              factory_(factory) {}
    virtual ~Domain() = default;

    Value value_;
    Type type_;
    DomainFactory* factory_;
};

std::ostream& operator<<(std::ostream& s, Domain::Ptr d);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, Domain::Ptr d);

struct DomainEquals {
    bool operator()(Domain::Ptr lhv, Domain::Ptr rhv) const noexcept {
        return lhv->equals(rhv.get());
    }
};

struct DomainHash {
    size_t operator()(Domain::Ptr lhv) const noexcept {
        return lhv->hashCode();
    }
};

struct Split {
    Domain::Ptr true_;
    Domain::Ptr false_;

    Split swap() {
        return {false_, true_};
    }
};

}   /* namespace absint */
}   /* namespace borealis*/

namespace std {

template <>
struct hash<borealis::absint::Domain::Ptr> {
    size_t operator()(const borealis::absint::Domain::Ptr& domain) const noexcept {
        return domain->hashCode();
    }
};

}   /* namespace std */

#endif //BOREALIS_DOMAIN_HPP

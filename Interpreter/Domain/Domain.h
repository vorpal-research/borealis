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

#include "Logging/logger.hpp"
#include "Type/Type.h"

namespace borealis {
namespace absint {

struct Split;

class DomainFactory;

enum BinaryOperator {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    SHL,
    SHR,
    LSHR,
    AND,
    OR,
    XOR
};

enum CmpOperator {
    EQ,
    NEQ,
    LT,
    LE,
    GT,
    GE
};

enum CastOperator {
    TRUNC,
    EXT,
    SEXT,
    FPTOI,
    ITOFP,
    ITOPTR,
    PTRTOI
};

class AbstractDomain : public ClassTag, public std::enable_shared_from_this<AbstractDomain> {
public:
    using Ptr = std::shared_ptr<AbstractDomain>;
    using ConstPtr = std::shared_ptr<const AbstractDomain>;

    explicit AbstractDomain(id_t id) : ClassTag(id) {}
    AbstractDomain(const AbstractDomain&) noexcept = default;
    AbstractDomain(AbstractDomain&&) noexcept = default;
    AbstractDomain& operator=(const AbstractDomain&) noexcept = default;
    AbstractDomain& operator=(AbstractDomain&&) noexcept = default;
    virtual ~AbstractDomain() = default;

    virtual bool isTop() const = 0;
    virtual bool isBottom() const = 0;

    virtual void setTop() = 0;
    virtual void setBottom() = 0;

    virtual bool leq(ConstPtr other) const = 0;
    virtual bool equals(ConstPtr other) const = 0;

    bool operator==(ConstPtr other) const { return this->equals(other); }
    bool operator!=(ConstPtr other) const { return not this->equals(other); }

    virtual void joinWith(ConstPtr other) = 0;
    virtual void meetWith(ConstPtr other) = 0;
    virtual void widenWith(ConstPtr other) = 0;

    virtual Ptr join(ConstPtr other) const = 0;
    virtual Ptr meet(ConstPtr other) const = 0;
    virtual Ptr widen(ConstPtr other) const = 0;

    virtual size_t hashCode() const = 0;
    virtual std::string toString() const = 0;

    static bool classof(const AbstractDomain*) {
        return true;
    }
};

class Domain : public std::enable_shared_from_this<Domain>, public logging::ObjectLevelLogging<Domain> {
public:

    enum DomainType {
        INTEGER_INTERVAL = 0,
        FLOAT_INTERVAL,
        NULLPTR,
        POINTER,
        FUNCTION,
        AGGREGATE
    };

    enum Value {
        TOP, VALUE, BOTTOM
    };

    using Ptr = std::shared_ptr<Domain>;

    /// Poset
    virtual bool equals(const Domain* other) const = 0;

    virtual bool operator<(const Domain& other) const = 0;

    virtual bool operator==(const Domain& other) const {
        if (this == &other) return true;
        else return this->equals(&other);
    }

    virtual bool operator!=(const Domain& other) const {
        if (this == &other) return false;
        else return not this->equals(&other);
    }

    virtual bool operator<=(const Domain& other) const {
        return ((*this) < other) || ((*this) == other);
    }

    /// Lattice
protected:
    virtual void setTop();

    virtual void setBottom();

    virtual void setValue();

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
    virtual void moveToTop();

    virtual Domain::Ptr join(Domain::Ptr other) = 0;

    virtual Domain::Ptr meet(Domain::Ptr other) = 0;

    virtual Domain::Ptr widen(Domain::Ptr other) = 0;

    /// Other
    virtual Domain::Ptr clone() const = 0;

    virtual size_t hashCode() const = 0;

    virtual std::string toPrettyString(const std::string& prefix) const {
        return prefix + "unknown";
    }

    virtual std::string toString() const {
        return toPrettyString("");
    }

    virtual DomainType getType() const {
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

    virtual Domain::Ptr implies(Domain::Ptr other) const;

    /// Unary
    virtual Domain::Ptr neg() const;

    virtual Domain::Ptr bNot() const;

    /// Vector
    virtual Domain::Ptr extractElement(const std::vector<Domain::Ptr>& indices);

    virtual void insertElement(Domain::Ptr element, const std::vector<Domain::Ptr>& indices);

    /// Aggregate
    virtual Domain::Ptr extractValue(Type::Ptr type, const std::vector<Domain::Ptr>& indices);

    virtual void insertValue(Domain::Ptr element, const std::vector<Domain::Ptr>& indices);

    /// Memory
    /// @arg type - type of the result element
    virtual Domain::Ptr load(Type::Ptr type, Domain::Ptr offset);

    virtual void store(Domain::Ptr value, Domain::Ptr offset);

    /// @arg type - type of the element that we want to get pointer to
    virtual Domain::Ptr gep(Type::Ptr type, const std::vector<Domain::Ptr>& indices);

    /// Cast
    /// Simple type casts are constant, they return new domain by definition
    virtual Domain::Ptr trunc(Type::Ptr type) const;

    virtual Domain::Ptr zext(Type::Ptr type) const;

    virtual Domain::Ptr sext(Type::Ptr type) const;

    virtual Domain::Ptr fptrunc(Type::Ptr type) const;

    virtual Domain::Ptr fpext(Type::Ptr type) const;

    virtual Domain::Ptr fptoui(Type::Ptr type) const;

    virtual Domain::Ptr fptosi(Type::Ptr type) const;

    virtual Domain::Ptr uitofp(Type::Ptr type) const;

    virtual Domain::Ptr sitofp(Type::Ptr type) const;

    virtual Domain::Ptr inttoptr(Type::Ptr type) const;

    /// Complicated casts are not constant, because they can change something inside of domain
    virtual Domain::Ptr ptrtoint(Type::Ptr type);

    virtual Domain::Ptr bitcast(Type::Ptr type);

    /// Other
    virtual Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;

    virtual Domain::Ptr fcmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const;

    /// Split operations
    virtual Split splitByEq(Domain::Ptr other);

    virtual Split splitByLess(Domain::Ptr other);

    virtual Split splitBySLess(Domain::Ptr other);

protected:

    Domain(Domain::Value value, Domain::DomainType type, DomainFactory* factory)
            : ObjectLevelLogging("domain"),
              value_(value),
              type_(type),
              factory_(factory) {}

    virtual ~Domain() = default;

    Value value_;
    const DomainType type_;
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
struct hash<std::shared_ptr<borealis::absint::AbstractDomain>> {
    size_t operator()(const std::shared_ptr<borealis::absint::AbstractDomain>& dom) const noexcept {
        return dom->hashCode();
    }
};

template <>
struct hash<borealis::absint::Domain::Ptr> {
    size_t operator()(const borealis::absint::Domain::Ptr& domain) const noexcept {
        return domain->hashCode();
    }
};

}   /* namespace std */

#endif //BOREALIS_DOMAIN_HPP

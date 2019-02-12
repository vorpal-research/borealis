//
// Created by abdullin on 2/11/19.
//

#ifndef BOREALIS_ABSTRACTDOMAIN_HPP
#define BOREALIS_ABSTRACTDOMAIN_HPP

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
    GE,
    FALSE,
    TRUE,
    BOTTOM,
    TOP
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

    ///////////////////////////////////////
    virtual Ptr apply(BinaryOperator op, ConstPtr other) const;

    virtual Ptr apply(CmpOperator op, ConstPtr other) const;

    virtual Ptr load(Type::Ptr type, Ptr offset) const;

    virtual void store(Ptr value, Ptr offset);

    virtual Ptr gep(Type::Ptr type, const std::vector <Ptr>& offsets);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

inline AbstractDomain::Ptr operator+(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::ADD, rhv);
}

inline AbstractDomain::Ptr operator-(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::SUB, rhv);
}

inline AbstractDomain::Ptr operator*(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::MUL, rhv);
}

inline AbstractDomain::Ptr operator/(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::DIV, rhv);
}

inline AbstractDomain::Ptr operator%(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::MOD, rhv);
}

inline AbstractDomain::Ptr operator<<(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::SHL, rhv);
}

inline AbstractDomain::Ptr operator>>(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::SHR, rhv);
}

inline AbstractDomain::Ptr lshr(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::LSHR, rhv);
}

inline AbstractDomain::Ptr operator&(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::AND, rhv);
}

inline AbstractDomain::Ptr operator|(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::OR, rhv);
}

inline AbstractDomain::Ptr operator^(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(BinaryOperator::XOR, rhv);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace absint
} // namespace borealis

namespace std {

template <>
struct hash<std::shared_ptr<borealis::absint::AbstractDomain>> {
    size_t operator()(const std::shared_ptr<borealis::absint::AbstractDomain>& dom) const noexcept {
        return dom->hashCode();
    }
};

}   /* namespace std */

#endif //BOREALIS_ABSTRACTDOMAIN_HPP

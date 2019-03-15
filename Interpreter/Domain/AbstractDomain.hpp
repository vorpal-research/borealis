//
// Created by abdullin on 2/11/19.
//

#ifndef BOREALIS_ABSTRACTDOMAIN_HPP
#define BOREALIS_ABSTRACTDOMAIN_HPP

#include "Logging/logger.hpp"
#include "Type/Type.h"
#include "Util/util.h"

namespace borealis {
namespace absint {

enum CastOperator {
    SIGN,
    TRUNC,
    EXT,
    SEXT,
    FPTOI,
    ITOFP,
    ITOPTR,
    PTRTOI,
    BITCAST
};

struct Split;

class AbstractDomain : public ClassTag, public logging::ObjectLevelLogging<AbstractDomain>,
        public std::enable_shared_from_this<AbstractDomain> {
public:
    using Ptr = std::shared_ptr<AbstractDomain>;
    using ConstPtr = std::shared_ptr<const AbstractDomain>;

    explicit AbstractDomain(id_t id) : ClassTag(id), ObjectLevelLogging("domain") {}

    AbstractDomain(const AbstractDomain&) = default;

    AbstractDomain(AbstractDomain&&) noexcept = default;

    AbstractDomain& operator=(const AbstractDomain&) = default;

    AbstractDomain& operator=(AbstractDomain&&) noexcept = default;

    virtual ~AbstractDomain() = default;

    virtual Ptr clone() const = 0;

    virtual bool isTop() const = 0;

    virtual bool isBottom() const = 0;

    virtual void setTop() = 0;

    virtual void setBottom() = 0;

    virtual bool leq(ConstPtr other) const = 0;

    virtual bool equals(ConstPtr other) const = 0;

    bool operator==(ConstPtr other) const { return this->equals(other); }

    bool operator!=(ConstPtr other) const { return not this->equals(other); }

//    virtual void joinWith(ConstPtr other) = 0;
//
//    virtual void meetWith(ConstPtr other) = 0;
//
//    virtual void widenWith(ConstPtr other) = 0;

    virtual Ptr join(ConstPtr other) const = 0;

    virtual Ptr meet(ConstPtr other) const = 0;

    virtual Ptr widen(ConstPtr other) const = 0;

    virtual size_t hashCode() const = 0;

    virtual std::string toString() const = 0;

    static bool classof(const AbstractDomain*) {
        return true;
    }

    ///////////////////////////////////////
    virtual Ptr apply(llvm::ArithType op, ConstPtr other) const;

    virtual Ptr apply(llvm::ConditionType op, ConstPtr other) const;

    virtual Ptr load(Type::Ptr type, Ptr offset) const;

    virtual void store(Ptr value, Ptr offset);

    virtual Ptr gep(Type::Ptr type, const std::vector <Ptr>& offsets);

    ///////////////////////////////////////
    virtual Split splitByEq(ConstPtr other) const;
    virtual Split splitByLess(ConstPtr other) const;
};

struct Split {
    AbstractDomain::Ptr true_;
    AbstractDomain::Ptr false_;

    Split swap() const {
        return { false_, true_ };
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

inline AbstractDomain::Ptr operator+(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::ADD, rhv);
}

inline AbstractDomain::Ptr operator-(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::SUB, rhv);
}

inline AbstractDomain::Ptr operator*(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::MUL, rhv);
}

inline AbstractDomain::Ptr operator/(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::DIV, rhv);
}

inline AbstractDomain::Ptr operator%(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::REM, rhv);
}

inline AbstractDomain::Ptr operator<<(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::SHL, rhv);
}

inline AbstractDomain::Ptr operator>>(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::ASHR, rhv);
}

inline AbstractDomain::Ptr lshr(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::LSHR, rhv);
}

inline AbstractDomain::Ptr operator&(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::BAND, rhv);
}

inline AbstractDomain::Ptr operator|(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::BOR, rhv);
}

inline AbstractDomain::Ptr operator^(AbstractDomain::ConstPtr lhv, AbstractDomain::ConstPtr rhv) {
    return lhv->apply(llvm::ArithType::XOR, rhv);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& s, AbstractDomain::Ptr domain);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, AbstractDomain::Ptr domain);

struct AbstractDomainHash {
    size_t operator()(AbstractDomain::Ptr d) const noexcept {
        return d->hashCode();
    }
};

struct AbstractDomainEquals {
    bool operator()(AbstractDomain::Ptr lhv, AbstractDomain::Ptr rhv) const noexcept {
        return lhv->equals(rhv);
    }
};

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

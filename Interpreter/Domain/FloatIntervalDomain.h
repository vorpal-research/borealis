//
// Created by abdullin on 2/17/17.
//

#ifndef BOREALIS_FLOATINTERVAL_H
#define BOREALIS_FLOATINTERVAL_H

#include "Interpreter/Domain/Domain.h"
#include "Interpreter/Domain/IntervalWidening.hpp"
#include "Interpreter/Util.hpp"
#include "Util/hash.hpp"

namespace borealis {
namespace absint {

class FloatIntervalDomain : public Domain {
public:

    /// Structure that identifies float interval
    using ID = std::tuple<Domain::Value, llvm::APFloat, llvm::APFloat>;
    struct IDHash;
    struct IDEquals;

public:

    FloatIntervalDomain(DomainFactory* factory, const llvm::APFloat& constant);
    FloatIntervalDomain(DomainFactory* factory, const llvm::APFloat& lb, const llvm::APFloat& ub);
    FloatIntervalDomain(DomainFactory* factory, const ID& id);
    FloatIntervalDomain(const FloatIntervalDomain& other);

    /// Poset
    bool equals(const Domain* other) const override;
    bool operator<(const Domain& other) const override;

    /// Lattice
    Domain::Ptr join(Domain::Ptr other) override;
    Domain::Ptr meet(Domain::Ptr other) override;
    Domain::Ptr widen(Domain::Ptr other) override;

    const llvm::fltSemantics& getSemantics() const;
    llvm::APFloat::roundingMode getRoundingMode() const;
    bool isConstant() const;
    bool isNaN() const;
    const llvm::APFloat& lb() const;
    const llvm::APFloat& ub() const;
    bool hasIntersection(const FloatIntervalDomain* other) const;

    Domain::Ptr clone() const override;
    size_t hashCode() const override;
    std::string toPrettyString(const std::string& prefix) const override;

    static bool classof(const Domain* other);

    /// Semantics
    Domain::Ptr fadd(Domain::Ptr other) const override;
    Domain::Ptr fsub(Domain::Ptr other) const override;
    Domain::Ptr fmul(Domain::Ptr other) const override;
    Domain::Ptr fdiv(Domain::Ptr other) const override;
    Domain::Ptr frem(Domain::Ptr other) const override;
    /// Cast
    Domain::Ptr fptrunc(const llvm::Type& type) const override;
    Domain::Ptr fpext(const llvm::Type& type) const override;
    Domain::Ptr fptoui(const llvm::Type& type) const override;
    Domain::Ptr fptosi(const llvm::Type& type) const override;
    Domain::Ptr bitcast(const llvm::Type& type) override;
    /// Other
    Domain::Ptr fcmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const override;
    /// Split operations
    Split splitByEq(Domain::Ptr other) override;
    Split splitByLess(Domain::Ptr other) override;

private:

    const llvm::APFloat lb_;
    const llvm::APFloat ub_;
    const FloatWidening* wm_;
};

struct FloatIntervalDomain::IDHash {
    size_t operator()(const ID& id) const {
        return std::hash<ID>()(id);
    }
};

struct FloatIntervalDomain::IDEquals {
    bool operator()(const ID& lhv, const ID& rhv) const {
        return std::get<0>(lhv) == std::get<0>(rhv) &&
               util::eq(std::get<1>(lhv), std::get<1>(rhv)) &&
               util::eq(std::get<2>(lhv), std::get<2>(rhv));
    }
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_FLOATINTERVAL_H

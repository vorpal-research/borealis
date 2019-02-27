//
// Created by abdullin on 2/26/19.
//

#ifndef BOREALIS_AGGREGATEDOMAIN_HPP
#define BOREALIS_AGGREGATEDOMAIN_HPP

#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Aggregate.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Inner, typename Variable>
class AggregateDomain : public Aggregate<Variable> {
public:
    using Self = AggregateDomain<Inner, Variable>;
    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using EnvT = SeparateDomain<Variable, Inner>;
    using EnvPtr = typename EnvT::Ptr;

protected:

    AbstractFactory* af_;
    EnvPtr env_;

private:

    explicit AggregateDomain(EnvPtr env) :
        Aggregate<Variable>(class_tag(*this)), af_(AbstractFactory::get()), env_(std::move(env)) {}

    static Self* unwrap(Ptr other) {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    static const Self* unwrap(ConstPtr other) {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

protected:

    EnvT* unwrapEnv() const {
        auto* envRaw = llvm::dyn_cast<EnvT>(env_.get());
        ASSERTC(envRaw);

        return envRaw;
    }

public:

    AggregateDomain() : AggregateDomain(EnvT::bottom()) {}
    AggregateDomain(const AggregateDomain&) = default;
    AggregateDomain(AggregateDomain&&) = default;
    AggregateDomain& operator=(const AggregateDomain&) = default;
    AggregateDomain& operator=(AggregateDomain&&) = default;
    virtual ~AggregateDomain() = default;

    static bool classof (const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override { return this->env_->isTop(); }
    bool isBottom() const override { return this->env_->isBottom(); }
    void setTop() override { this->env_->setTop(); }
    void setBottom() override { this->env_->setBottom(); }

    bool leq(ConstPtr other) const override { return this->env_->leq(unwrap(other)->env_); }
    bool equals(ConstPtr other) const override { return this->env_->equals(unwrap(other)->env_); }

    void joinWith(ConstPtr other) override { this->env_->joinWith(unwrap(other)->env_); }
    void meetWith(ConstPtr other) override { this->env_->meetWith(unwrap(other)->env_); }
    void widenWith(ConstPtr other) override { this->env_->widenWith(unwrap(other)->env_); }

    Ptr join(ConstPtr other) const override {
        auto&& result = this->clone();
        result->joinWith(other);
        return result;
    }

    Ptr meet(ConstPtr other) const override {
        auto&& result = this->clone();
        result->meetWith(other);
        return result;
    }

    Ptr widen(ConstPtr other) const override {
        auto&& result = this->clone();
        result->widenWith(other);
        return result;
    }

    void set(Variable x, Ptr value) { return unwrapEnv()->set(x, value); }
    void forget(Variable x) { return unwrapEnv()->forget(x); }

    void assign(Variable x, Ptr i) override { this->set(x, i); }

    Ptr extractFrom(Type::Ptr type, Variable ptr, Ptr index) const override {
        return this->get(ptr)->load(type, index);
    }

    void insertTo(Variable ptr, Ptr value, Ptr index) override {
        this->get(ptr)->store(value, index);
    }

    size_t hashCode() const override {
        return this->env_->hashCode();
    }

    std::string toString() const override {
        return env_->toString();
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_AGGREGATEDOMAIN_HPP

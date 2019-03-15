//
// Created by abdullin on 2/13/19.
//

#ifndef BOREALIS_POINTSTODOMAIN_HPP
#define BOREALIS_POINTSTODOMAIN_HPP

#include "Interpreter/Domain/AbstractFactory.hpp"

#include "PointerDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename MachineInt, typename Variable>
class PointsToDomain : public MemoryDomain<MachineInt, Variable> {
public:
    using Self = PointsToDomain<MachineInt, Variable>;
    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using EnvT = SeparateDomain<Variable, PointerDomain<MachineInt>>;
    using EnvPtr = typename EnvT::Ptr;

protected:

    AbstractFactory* af_;
    EnvPtr env_;

private:

    explicit PointsToDomain(EnvPtr env) :
            MemoryDomain<MachineInt, Variable>(class_tag(*this)), af_(AbstractFactory::get()), env_(std::move(env)) {}

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

    PointsToDomain() : PointsToDomain(EnvT::bottom()) {}
    PointsToDomain(const PointsToDomain&) = default;
    PointsToDomain(PointsToDomain&&) = default;
    PointsToDomain& operator=(const PointsToDomain&) = default;
    PointsToDomain& operator=(PointsToDomain&&) = default;
    virtual ~PointsToDomain() = default;

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

    void joinWith(ConstPtr other) { this->env_ = this->env_->join(unwrap(other)->env_); }
    void meetWith(ConstPtr other) { this->env_ = this->env_->meet(unwrap(other)->env_); }
    void widenWith(ConstPtr other) { this->env_ = this->env_->widen(unwrap(other)->env_); }

    Ptr join(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->joinWith(other);
        return result;
    }

    Ptr meet(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->meetWith(other);
        return result;
    }

    Ptr widen(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->widenWith(other);
        return result;
    }

    void set(Variable x, Ptr value) { return unwrapEnv()->set(x, value); }
    void forget(Variable x) { return unwrapEnv()->forget(x); }

    void assign(Variable x, Ptr i) override { this->set(x, i); }

    Ptr applyTo(llvm::ConditionType, Variable, Variable) const override {
        auto&& makeTop = [&]() { return af_->getBool(AbstractFactory::TOP); };
        return makeTop();
    }

    Ptr loadFrom(Type::Ptr type, Variable ptr) const override {
        auto&& ptrDom = this->get(ptr);
        return ptrDom->load(type, af_->getMachineInt(0));
    }

    void storeTo(Variable ptr, Ptr x) override {
        auto&& ptrDom = this->get(ptr);
        ptrDom->store(x, af_->getMachineInt(0));
    }

    void gepFrom(Variable x, Type::Ptr type, Variable ptr, const std::vector<Ptr>& shifts) override {
        auto&& ptrDom = this->get(ptr);
        auto&& xDom = ptrDom->gep(type, shifts);
        assign(x, xDom);
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

#endif //BOREALIS_POINTSTODOMAIN_HPP

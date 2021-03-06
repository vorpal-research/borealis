//
// Created by abdullin on 2/8/19.
//

#ifndef BOREALIS_FUNCTIONDOMAIN_HPP
#define BOREALIS_FUNCTIONDOMAIN_HPP

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename FunctionT, typename FHash, typename FEquals>
class FunctionDomain : public AbstractDomain {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;
    using FunctionSet = std::unordered_set<FunctionT, FHash, FEquals>;

    using Self = FunctionDomain<FunctionT, FHash, FEquals>;

private:

    bool isBottom_;
    Type::Ptr prototype_;
    AbstractFactory* factory_;
    FunctionSet functions_;

private:
    struct TopTag{};
    struct BottomTag{};

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

public:

    FunctionDomain(TopTag, Type::Ptr type) :
            AbstractDomain(class_tag(*this)), isBottom_(false), prototype_(type), factory_(AbstractFactory::get()) {}

    FunctionDomain(BottomTag, Type::Ptr type) :
            AbstractDomain(class_tag(*this)), isBottom_(true), prototype_(type), factory_(AbstractFactory::get()) {}

    explicit FunctionDomain(Type::Ptr type) : FunctionDomain(TopTag{}, type) {}
    explicit FunctionDomain(FunctionT function) :
            AbstractDomain(class_tag(*this)), isBottom_(false), prototype_(function->getType()),
            factory_(AbstractFactory::get()), functions_({ function }) {}

    FunctionDomain(const FunctionDomain&) = default;
    FunctionDomain(FunctionDomain&&) = default;
    FunctionDomain& operator=(FunctionDomain&&) = default;
    FunctionDomain& operator=(const FunctionDomain& other) {
        if (this != &other) {
            this->isBottom_ = other.isBottom_;
            this->prototype_ = other.prototype_;
            this->factory_ = other.factory_;
            this->functions_ = other.functions_;
        }
        return *this;
    }

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    const FunctionSet& functions() const {
        return this->functions_;
    }

    static Ptr top(Type::Ptr type) { return std::make_shared<Self>(TopTag{}, type); }
    static Ptr bottom(Type::Ptr type) { return std::make_shared<Self>(BottomTag{}, type); }
    static Ptr constant(FunctionT function) { return std::make_shared<Self>(function); }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override { return (not isBottom_) and functions_.empty(); }
    bool isBottom() const override { return isBottom_; }

    void setTop() override {
        isBottom_ = false;
        functions_.clear();
    }

    void setBottom() override {
        isBottom_ = true;
        functions_.clear();
    }

    bool leq(ConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(ConstPtr other) const override {
        if (this == other.get()) return true;

        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        if (not otherRaw) return false;

        if (this->isTop()) {
            return other->isTop();
        } else if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isTop() || other->isBottom()) {
            return false;
        } else {
            if (this->functions_.size() != otherRaw->functions_.size()) return false;
            return util::equal_with_find(this->functions_, otherRaw->functions_,
                                         [](auto&& a) { return a; },
                                         [](auto&& a, auto&& b) { return a->equals(b.get()); });
        }
    }

    void joinWith(ConstPtr other) {
        if (this == other.get()) return;
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else if (this->isTop()) {
            return;
        } else if (other->isBottom()) {
            return;
        } else if (other->isTop()) {
            this->setTop();
        } else {

            for (auto&& it : otherRaw->functions_) {
                this->functions_.insert(it);
            }
        }
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (this->isTop()) {
            this->operator=(*otherRaw);
        } else if (other->isBottom()) {
            this->setBottom();
        } else if (other->isTop()) {
            return;
        } else {

            for (auto&& it : this->functions_) {
                if (not util::contains(otherRaw->functions_, it)) {
                    this->functions_.erase(it);
                }
            }
        }
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    void widenWith(ConstPtr other) {
        joinWith(other);
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    size_t hashCode() const override { return class_tag(*this); } //return util::hash::defaultHasher()(isBottom_); }
    std::string toString() const override {
        std::ostringstream ss;
        ss << "Function " << TypeUtils::toString(*prototype_.get()) << " : ";
        if (this->isTop()) ss << "[ TOP ]";
        else if (this->isBottom()) ss << "[ BOTTOM ]";
        else {
            for (auto&& it : functions_) {
                ss << std::endl << "  " << it->getName();
            }
            ss << std::endl << "]";
        }
        return ss.str();
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_FUNCTIONDOMAIN_HPP

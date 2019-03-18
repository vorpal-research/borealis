//
// Created by abdullin on 1/28/19.
//

#ifndef BOREALIS_SEPARATE_DOMAIN_HPP
#define BOREALIS_SEPARATE_DOMAIN_HPP

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Util/collections.hpp"
#include "Util/cow_map.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Variable, typename VarHash, typename VarEquals, typename Inner>
class SeparateDomain : public AbstractDomain {
public:

    template <typename Key, typename Value>
    using VariableMapImpl = std::unordered_map<Key, Value, VarHash, VarEquals>;

    using Self = SeparateDomain<Variable, VarHash, VarEquals, Inner>;
    using ValueMap = util::cow_map<Variable, typename Inner::Ptr, VariableMapImpl>;
    using Iterator = typename ValueMap::iterator;

private:

    ValueMap values_;
    bool isBottom_;

private:
    struct TopTag {};
    struct BottomTag {};

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

    explicit SeparateDomain(TopTag) : AbstractDomain(class_tag(*this)), isBottom_(false) {}
    explicit SeparateDomain(BottomTag) : AbstractDomain(class_tag(*this)), isBottom_(true) {}

    SeparateDomain() : SeparateDomain(TopTag{}) {}
    SeparateDomain(const SeparateDomain&) = default;
    SeparateDomain(SeparateDomain&&) = default;
    SeparateDomain& operator=(SeparateDomain&&) = default;
    SeparateDomain& operator=(const SeparateDomain& other) {
        if (this != &other) {
            this->values_ = other.values_;
            this->isBottom_ = other.isBottom_;
        }
        return *this;
    }

    ~SeparateDomain() override = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof (const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    static Ptr top() { return std::make_shared<Self>(TopTag{}); }
    static Ptr bottom() { return std::make_shared<Self>(BottomTag{}); }

    bool isTop() const override { return !this->isBottom() && this->values_.empty(); }
    bool isBottom() const override { return this->isBottom_; }

    void setTop() override {
        this->values_.clear();
        this->isBottom_ = false;
    }

    void setBottom() override {
        this->values_.clear();
        this->isBottom_ = true;
    }

    bool leq(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return true;
        } else if (otherRaw->isBottom()) {
            return false;
        } else {
            for (auto&& it : this->values_) {
                auto&& otherIt = otherRaw->values_.find(it.first);
                if (otherIt == otherRaw->values_.end()) return false;
                else if (not it.second->equals((*otherIt).second)) return false;
            }
            return true;
        }
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        if (not otherRaw) return false;

        if (this->isBottom()) {
            return otherRaw->isBottom();
        } else if (otherRaw->isBottom()) {
            return false;
        } else {
            if (this->values_.size() != otherRaw->values_.size()) return false;

            for (auto&& it : this->values_) {
                auto&& otherIt = otherRaw->values_.find(it.first);
                if (otherIt == otherRaw->values_.end()) return false;
                else if (not it.second->equals((*otherIt).second)) return false;
            }
            return true;
        }
    }

    void joinWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (otherRaw->isBottom()) {
            return;
        } else if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else {
            for (auto&& it : otherRaw->values_) {
                auto&& cur = values_.find(it.first);
                if (cur == values_.end()) values_[it.first] = it.second;
                else values_[it.first] = cur->second->join(it.second);
            }
            return;
        }
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (otherRaw->isBottom()) {
            this->setBottom();
        } else {
            for (auto&& it : otherRaw->values_) {
                auto&& cur = values_.find(it.first);
                if (cur == values_.end()) values_[it.first] = it.second;
                else values_[it.first] = cur->second->meet(it.second);
            }
            return;
        }
    }

    void widenWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (otherRaw->isBottom()) {
            return;
        } else if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else {
            for (auto&& it : otherRaw->values_) {
                auto&& cur = values_.find(it.first);
                if (cur == values_.end()) values_[it.first] = it.second;
                else values_[it.first] = cur->second->widen(it.second);
            }
            return;
        }
    }

    Ptr join(ConstPtr other) const override {
        auto&& result = std::make_shared<Self>(*this);
        result->joinWith(other);
        return result;
    }

    Ptr meet(ConstPtr other) const override {
        auto&& result = std::make_shared<Self>(*this);
        result->meetWith(other);
        return result;
    }

    Ptr widen(ConstPtr other) const override {
        auto&& result = std::make_shared<Self>(*this);
        result->widenWith(other);
        return result;
    }

    typename Inner::Ptr get(Variable x,
            const std::function<typename Inner::Ptr()>&,
            const std::function<typename Inner::Ptr()>&) const {
        auto&& opt = util::at(this->values_, x);
        if (opt.empty()) return nullptr;
        else return opt.getUnsafe();
    }

    void set(Variable x, typename Inner::Ptr value) {
        this->values_[x] = value;
        this->isBottom_ = false;
    }

    void forget(Variable x) {
        if (this->isBottom()) {
            return;
        }
        this->values_.erase(x);
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(isBottom_);
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "{" << std::endl;
        if (this->isBottom()) ss << " BOTTOM ";
        for (auto&& it : values_) {
            ss << util::toString(*it.first) << " = " << it.second->toString() << std::endl;
        }
        ss << "}";
        return ss.str();
    }

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_SEPARATE_DOMAIN_HPP

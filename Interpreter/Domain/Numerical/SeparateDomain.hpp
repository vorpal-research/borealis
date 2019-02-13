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

template <typename Number, typename Variable, typename Inner>
class SeparateDomain : public AbstractDomain {
public:

    using Self = SeparateDomain<Number, Variable, Inner>;
    using ValueMap = util::cow_map<Variable, typename Inner::Ptr>;
    using Iterator = typename ValueMap::iterator;

private:

    ValueMap values_;
    bool isBottom_;

private:
    struct TopTag {};
    struct BottomTag {};

    explicit SeparateDomain(TopTag) : AbstractDomain(class_tag(*this)), isBottom_(false) {}
    explicit SeparateDomain(BottomTag) : AbstractDomain(class_tag(*this)), isBottom_(true) {}

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

    SeparateDomain() : SeparateDomain(TopTag{}) {}
    SeparateDomain(const SeparateDomain&) = default;
    SeparateDomain(SeparateDomain&&) noexcept = default;
    SeparateDomain& operator=(const SeparateDomain&) = default;
    SeparateDomain& operator=(SeparateDomain&&) noexcept = default;
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

    static Ptr top() { return SeparateDomain(TopTag{}); }
    static Ptr bottom() { return SeparateDomain(BottomTag{}); }

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
            return util::equal_with_find(this->values_, otherRaw->values_,
                    LAM(a, a.first), LAM2(a, b, a.second->leq(b.second)));
        }
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return otherRaw->isBottom();
        } else if (otherRaw->isBottom()) {
            return false;
        } else {
            return util::equal_with_find(this->values_, otherRaw->values_,
                                         LAM(a, a.first), LAM2(a, b, a.second->equals(b.second)));
        }
    }

    void joinWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        if (otherRaw->isBottom()) {
            return;
        } else if (this->isBottom()) {
            return this->operator=(*otherRaw);
        } else {
            for (auto&& it : otherRaw->values_) {
                auto&& cur = values_.find(it.first);
                if (cur == values_.end()) values_[it.first] = it.second;
                else values_[it.first] = cur->join(it.second);
            }
            return;
        }
    }

    void meetWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (otherRaw->isBottom()) {
            this->setBottom();
        } else {
            for (auto&& it : otherRaw->values_) {
                auto&& cur = values_.find(it.first);
                if (cur == values_.end()) values_[it.first] = it.second;
                else values_[it.first] = cur->meet(it.second);
            }
            return;
        }
    }

    void widenWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        if (otherRaw->isBottom()) {
            return;
        } else if (this->isBottom()) {
            return this->operator=(*otherRaw);
        } else {
            for (auto&& it : otherRaw->values_) {
                auto&& cur = values_.find(it.first);
                if (cur == values_.end()) values_[it.first] = it.second;
                else values_[it.first] = cur->widen(it.second);
            }
            return;
        }
    }

    typename Inner::Ptr get(Variable x) const {
        if (this->isBottom()) {
            return Inner::bottom();
        } else {
            auto&& opt = util::at(this->values_, x);
            if (opt.empty()) return Inner::top();
            else return opt.getUnsafe();
        }
    }

    void set(Variable x, typename Inner::ConstPtr value) {
        if (this->isBottom()) {
            return;
        } else if (value->isBottom()) {
            this->setBottom();
        } else if (value->isTop()) {
            this->values_.erase(x);
        } else {
            this->values_[x] = value;
        }
    }

    void forget(Variable x) {
        if (this->isBottom()) {
            return;
        }
        this->values_.erase(x);
    }

    void apply(llvm::ArithType op, Variable x, Variable y, Variable z) {
        this->set(x, this->get(y)->apply(op, this->get(z)));
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_SEPARATE_DOMAIN_HPP

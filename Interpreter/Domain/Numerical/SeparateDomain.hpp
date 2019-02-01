//
// Created by abdullin on 1/28/19.
//

#ifndef BOREALIS_SEPARATE_DOMAIN_HPP
#define BOREALIS_SEPARATE_DOMAIN_HPP

#include "Interpreter/Domain/Domain.h"

#include "Util/collections.hpp"
#include "Util/cow_map.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Number, typename Variable, typename Inner>
class SeparateDomain : public AbstractDomain<SeparateDomain<Number, Variable, Inner>> {
public:
    using Ptr = std::shared_ptr<SeparateDomain<Number, Variable, Inner>>;
    using ConstPtr = std::shared_ptr<const SeparateDomain<Number, Variable, Inner>>;

    using ValueMap = util::cow_map<Variable, typename Inner::Ptr>;
    using Iterator = typename ValueMap::iterator;

private:

    ValueMap values_;
    bool isBottom_;

private:
    struct TopTag {};
    struct BottomTag {};

public:

    SeparateDomain() : SeparateDomain(TopTag{}) {}
    explicit SeparateDomain(TopTag) : isBottom_(false) {}
    explicit SeparateDomain(BottomTag) : isBottom_(true) {}
    SeparateDomain(const SeparateDomain&) = default;
    SeparateDomain(SeparateDomain&&) noexcept = default;
    SeparateDomain& operator=(const SeparateDomain&) = default;
    SeparateDomain& operator=(SeparateDomain&&) noexcept = default;
    ~SeparateDomain() override = default;

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
        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else {
            return util::equal_with_find(this->values_, other->values_,
                    LAM(a, a.first), LAM2(a, b, a.second->leq(b.second)));
        }
    }

    bool equals(ConstPtr other) const override {
        if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isBottom()) {
            return false;
        } else {
            return util::equal_with_find(this->values_, other->values_,
                                         LAM(a, a.first), LAM2(a, b, a.second->equals(b.second)));
        }
    }

    void joinWith(ConstPtr other) override {
        if (other->isBottom()) {
            return;
        } else if (this->isBottom()) {
            return this->operator=(*other.get());
        } else {
            for (auto&& it : other->values_) {
                auto&& cur = values_.find(it.first);
                if (cur == values_.end()) values_[it.first] = it.second;
                else values_[it.first] = cur->join(it.second);
            }
            return;
        }
    }

    void meetWith(ConstPtr other) override {
        if (this->isBottom()) {
            return;
        } else if (other->isBottom()) {
            this->setBottom();
        } else {
            for (auto&& it : other->values_) {
                auto&& cur = values_.find(it.first);
                if (cur == values_.end()) values_[it.first] = it.second;
                else values_[it.first] = cur->meet(it.second);
            }
            return;
        }
    }

    void widenWith(ConstPtr other) override {
        if (other->isBottom()) {
            return;
        } else if (this->isBottom()) {
            return this->operator=(*other.get());
        } else {
            for (auto&& it : other->values_) {
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

    void apply(BinaryOperator op, Variable x, Variable y, Variable z) {
        this->set(x, this->get(y)->apply(op, this->get(z)));
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_SEPARATE_DOMAIN_HPP

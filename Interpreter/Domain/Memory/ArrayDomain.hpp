//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_ARRAYDOMAIN_HPP
#define BOREALIS_ARRAYDOMAIN_HPP

#include <unordered_map>

#include "Interpreter/Domain/Domain.h"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename MachineInt, typename ElementT>
class ArrayDomain : public AbstractDomain<ArrayDomain<MachineInt, ElementT>> {
public:

    using Ptr = std::shared_ptr<ArrayDomain<MachineInt, ElementT>>;
    using ConstPtr = std::shared_ptr<const ArrayDomain<MachineInt, ElementT>>;

    using IntervalT = Interval<MachineInt>;
    using IntervalPtr = typename IntervalT::Ptr;
    using ElementPtr = typename ElementT::Ptr;
    using ElementMapT = std::unordered_map<size_t, ElementPtr>;

private:

    IntervalPtr length_;
    ElementMapT elements_;

private:
    struct TopTag {};
    struct BottomTag {};

    explicit ArrayDomain(TopTag) : length_(IntervalT::top()) {}
    explicit ArrayDomain(BottomTag) : length_(IntervalT::bottom()) {}

public:

    ArrayDomain() : ArrayDomain(TopTag{}) {}
    ArrayDomain(const ArrayDomain&) = default;
    ArrayDomain(ArrayDomain&&) = default;
    ArrayDomain& operator=(const ArrayDomain&) = default;
    ArrayDomain& operator=(ArrayDomain&&) = default;

    static Ptr top() { return std::make_shared<ArrayDomain>(TopTag{}); }
    static Ptr bottom() { return std::make_shared<ArrayDomain>(BottomTag{}); }

    bool isTop() const override { return length_->isTop() and elements_.empty(); }
    bool isBottom() const { return length_->isBottom() and elements_.empty(); }

    void setTop() override {
        length_->setTop();
        elements_.clear();
    }

    void setBottom() override {
        length_->setBottom();
        elements_.clear();
    }

    bool leq(ConstPtr other) const override {
        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else {
            return this->length_ < other->length_ and
            util::equal_with_find(this->elements_, other->elements_, LAM(a, a.first), LAM2(a, b, a.second->leq(b.second)));
        }
    }

    bool equals(ConstPtr other) const  override {
        if (this->isBottom()) {
            return other->isBottom();
        } else if (this->isTop()) {
            return other->isTop();
        } else if (other->isTop() || other->isBottom()) {
            return false;
        } else {

            if (this->length_ != other->length_) return false;

            for (auto&& it : this->elements_) {
                auto&& opt = util::at(other->elements_, it.first);
                if ((not opt) || (not it.second->equals(opt.getUnsafe().get()))) {
                    return false;
                }
            }

            return true;
        }
    }

    void joinWith(ConstPtr other) override {
        if (this->isBottom()) {
            this->operator=(*other.get());
        } else if (this->isTop()) {
            return;
        } else if (other->isBottom()) {
            return;
        } else if (other.isTop()) {
            this->operator=(*other.get());
        } else {

            this->length_->joinWith(other->length_);
            for (auto&& it : other->elements_) {
                auto&& opt = util::at(this->elements_, it.first);
                if (not opt) {
                    this->elements_[it.first] = it.second;
                } else {
                    this->elements_[it.first] = it.second->join(opt.getUnsafe());
                }
            }
        }
    }

    void meetWith(ConstPtr other) override {
        if (this->isBottom()) {
            return;
        } else if (this->isTop()) {
            this->operator=(other);
        } else if (other->isBottom()) {
            this->operator=(other);
        } else if (other.isTop()) {
            return;
        } else {

            this->length_->meetWith(other->length_);
            for (auto&& it : other->elements_) {
                auto&& opt = util::at(this->elements_, it.first);
                if (not opt) {
                    this->elements_[it.first] = it.second;
                } else {
                    this->elements_[it.first] = it.second->meet(opt.getUnsafe());
                }
            }
        }
    }

    void widenWith(ConstPtr other) override {
        if (this->isBottom()) {
            this->operator=(*other.get());
        } else if (this->isTop()) {
            return;
        } else if (other->isBottom()) {
            return;
        } else if (other.isTop()) {
            this->operator=(*other.get());
        } else {

            this->length_->widenWith(other->length_);
            for (auto&& it : other->elements_) {
                auto&& opt = util::at(this->elements_, it.first);
                if (not opt) {
                    this->elements_[it.first] = it.second;
                } else {
                    this->elements_[it.first] = it.second->widen(opt.getUnsafe());
                }
            }
        }
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(this->length_, this->elements_);
    }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "Array [" << this->length_->ub() << " x " << ElementT::name() << "] ";
        ss << ": [";
        if (this->isTop()) {
            ss << " TOP ]";
        } else if (this->isBottom()) {
            ss << " BOTTOM ]";
        } else {
            for (auto&& it : this->elements_) {
                ss << std::endl << "  " << it.first << " : " << it.second->toString();
            }
            ss << std::endl << "]";
        }
        return ss.str();
    }

    ElementPtr load(IntervalPtr interval) const {
        if (this->isTop()) {
            return ElementT::top();
        } else if (this->isBottom()) {
            return ElementT::bottom();
        } else {
            auto result = ElementT::bottom();

            auto lengthUb = this->length_->ub();
            auto lb = interval->lb();
            auto ub = interval->ub();

            if (ub >= lengthUb) {
                warns() << "Possible buffer overflow" << endl;
            } else if (lb >= lengthUb) {
                warns() << "Buffer overflow" << endl;
                return ElementT::top();
            }

            for (auto i = lb; i < ub and i < lengthUb; ++i) {
                auto&& opt = util::at(this->elements_, i);
                result.joinWith((not opt) ? ElementT::bottom() : opt.getUnsafe());
            }

            return result;
        }
    }

    void store(IntervalPtr interval, ElementPtr value) {
        if (this->isTop()) {
            return;
        } else if (this->isBottom()) {
            return;
        } else {
            auto lengthUb = this->length_->ub();
            auto lb = interval->lb();
            auto ub = interval->ub();

            if (ub >= lengthUb) {
                warns() << "Possible buffer overflow" << endl;
            } else if (lb >= lengthUb) {
                warns() << "Buffer overflow" << endl;
                return ElementT::top();
            }

            for (auto i = lb; i < ub and i < lengthUb; ++i) {
                auto&& opt = util::at(this->elements_, i);
                if (not opt) {
                    this->elements_[i.number()] = value;
                } else {
                    opt.getUnsafe()->joinWith(value);
                }
            }
        }
    }

};

} // namespace absint
} // namespace borealis

namespace std {

template <typename Derived>
struct hash<std::shared_ptr<borealis::absint::AbstractDomain<Derived>>> {
    size_t operator()(const std::shared_ptr<borealis::absint::AbstractDomain<Derived>>& dom) const noexcept {
        return dom->hashCode();
    }
};

} // namespace std

#include "Util/unmacros.h"

#endif //BOREALIS_ARRAYDOMAIN_HPP

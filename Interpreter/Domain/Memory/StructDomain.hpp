//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_STRUCTDOMAIN_HPP
#define BOREALIS_STRUCTDOMAIN_HPP

#include <unordered_map>

#include "Interpreter/Domain/Domain.h"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename ...Element>
class StructDomain : public AbstractDomain<StructDomain<Element...>> {
public:

    using Ptr = std::shared_ptr<StructDomain<Element...>>;
    using ConstPtr = std::shared_ptr<const StructDomain<Element...>>;

    using MachineIntT = IntNumber<64, false>;
    using IntervalT = Interval<MachineIntT>;
    using IntervalPtr = typename IntervalT::Ptr;
    using ElementsT = std::tuple<std::shared_ptr<Element>...>;

private:

    size_t length_;
    ElementsT elements_;

private:
    struct TopTag{};
    struct BottomTag{};

    explicit StructDomain(TopTag) : length_(sizeof...(Element)) {
        this->elements_ = std::make_tuple(Element::top()...);
    }

    explicit StructDomain(BottomTag) : length_(sizeof...(Element)) {
        this->elements_ = std::make_tuple(Element::bottom()...);
    }

public:

    StructDomain(Element... elems) : length_(sizeof...(Element)) {
        this->elements_ = std::forward_as_tuple(elems...);
    }

    StructDomain(const StructDomain&) = default;
    StructDomain(StructDomain&&) = default;
    StructDomain& operator=(const StructDomain&) = default;
    StructDomain& operator=(StructDomain&&) = default;

    static Ptr top() { return std::make_shared<StructDomain>(TopTag{}); }

    static Ptr bottom() { return std::make_shared<StructDomain>(BottomTag{}); }

    bool isTop() const override {
        bool top = true;
        for (auto i = 0U; i < length_; ++i) {
            top &= std::get<i>(elements_)->isTop();
        }
        return top;
    }

    bool isBottom() const override {
        bool top = true;
        for (auto i = 0U; i < length_; ++i) {
            top &= std::get<i>(elements_)->isBottom();
        }
        return top;
    }

    void setTop() override {
        this->elements_ = std::make_tuple(Element::top()...);
    }

    void setBottom() override {
        this->elements_ = std::make_tuple(Element::bottom()...);
    }

    bool leq(ConstPtr other) const override {
        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else {
            UNREACHABLE("Don't know how to implement");
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

            for (auto i = 0U; i < this->length_; ++i) {
                if (std::get<i>(this->elements_) != std::get<i>(other->elements_)) {
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

            ASSERT(this->length_ == other->length_, "trying to join different length structs");

            for (auto i = 0U; i < this->length_; ++i) {
                std::get<i>(this->elements_)->joinWith(std::get<i>(other->elements_));
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

            ASSERT(this->length_ == other->length_, "trying to meet different length structs");

            for (auto i = 0U; i < this->length_; ++i) {
                std::get<i>(this->elements_)->meetWith(std::get<i>(other->elements_));
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

            ASSERT(this->length_ == other->length_, "trying to widen different length structs");

            for (auto i = 0U; i < this->length_; ++i) {
                std::get<i>(this->elements_)->widenWith(std::get<i>(other->elements_));
            }
        }
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(this->length_, this->elements_);
    }

    std::string toString() const override {
        std::ostringstream ss;
        auto&& elemNames = std::make_tuple(Element::name()...);
        ss << "Struct [";
        ss << std::get<0>(elemNames);
        for (auto i = 1U; i < this->length_; ++i) {
            ss << ", " << std::get<i>(elemNames);
        }
        ss << "] ";
        ss << ": {";
        if (this->isTop()) {
            ss << " TOP }";
        } else if (this->isBottom()) {
            ss << " BOTTOM }";
        } else {
            for (auto i = 0U; i < this->length_; ++i) {
                ss << std::endl << "  " << std::get<i>(this->elements_)->toString();
            }
            ss << std::endl << "}";
        }
        return ss.str();
    }

    template <typename Elem>
    typename Elem::Ptr load(IntervalPtr interval) const {
        if (this->isTop()) {
            return Elem::top();
        } else if (this->isBottom()) {
            return Elem::bottom();
        } else {
            auto lb = interval->lb();

            auto result = Elem::bottom();
            auto ub = interval->ub();

            if (ub >= this->length_) {
                warns() << "Possible buffer overflow" << endl;
            } else if (lb >= this->length_) {
                warns() << "Buffer overflow" << endl;
                return Elem::top();
            }

            for (auto i = lb; i < ub and i < this->length_; ++i) {
                result.joinWith(std::get<i>(this->elements_));
            }

            return result;
        }
    }

    template <typename Elem>
    void store(IntervalPtr interval, typename Elem::Ptr value) {
        if (this->isTop()) {
            return;
        } else if (this->isBottom()) {
            return;
        } else {
            auto lb = interval->lb();
            auto ub = interval->ub();

            if (ub >= this->length_) {
                warns() << "Possible buffer overflow" << endl;
            } else if (lb >= this->length_) {
                warns() << "Buffer overflow" << endl;
                return Elem::top();
            }

            for (auto i = lb; i < ub and i < this->length_; ++i) {
                std::get<i>(this->elements_)->joinWith(value);
            }
        }
    }

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_STRUCTDOMAIN_HPP

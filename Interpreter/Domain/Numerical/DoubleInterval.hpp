//
// Created by abdullin on 2/13/19.
//

#ifndef BOREALIS_DOUBLEINTERVAL_HPP
#define BOREALIS_DOUBLEINTERVAL_HPP

#include "Interval.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename N1, typename N2>
class DoubleInterval : public AbstractDomain {
public:

    using Self = DoubleInterval<N1, N2>;
    using Interval1 = Interval<N1>;
    using Interval2 = Interval<N2>;

private:

    Ptr first_;
    Ptr second_;

private:

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

    void normalize() {
        if (first_->isTop() || second_->isTop()) setTop();
        else if (first_->isBottom() || second_->isBottom()) setBottom();
    }

public:
    struct TopTag {};
    struct BottomTag {};

    explicit DoubleInterval(TopTag) : AbstractDomain(class_tag(*this)), first_(Interval1::top()), second_(Interval2::top()) {}
    explicit DoubleInterval(BottomTag) : AbstractDomain(class_tag(*this)), first_(Interval1::bottom()), second_(Interval2::bottom()) {}

    DoubleInterval() : DoubleInterval(TopTag{}) {}
    explicit DoubleInterval(int n) :
            AbstractDomain(class_tag(*this)),
            first_(Interval1::constant(n)),
            second_(Interval2::constant(n)) {
        normalize();
    }

    explicit DoubleInterval(const N1& n) :
            AbstractDomain(class_tag(*this)),
            first_(Interval1::constant(n)),
            second_(Interval2::constant(n)) {
        normalize();
    }

    explicit DoubleInterval(const N2& n) :
            AbstractDomain(class_tag(*this)),
            first_(Interval1::constant(n)),
            second_(Interval2::constant(n)) {
        normalize();
    }

    DoubleInterval(int from, int to) :
        AbstractDomain(class_tag(*this)),
        first_(std::make_shared<Interval1>(from, to)),
        second_(std::make_shared<Interval2>(from, to)) {
        normalize();
    }

    DoubleInterval(const N1& from, const N1& to) :
            AbstractDomain(class_tag(*this)),
            first_(std::make_shared<Interval1>(from, to)),
            second_(std::make_shared<Interval2>(from, to)) {
        normalize();
    }

    DoubleInterval(const N2& from, const N2& to) :
            AbstractDomain(class_tag(*this)),
            first_(std::make_shared<Interval1>(from, to)),
            second_(std::make_shared<Interval2>(from, to)) {
        normalize();
    }

    DoubleInterval(Ptr sint, Ptr uint) :
            AbstractDomain(class_tag(*this)),
            first_(sint),
            second_(uint) {
        normalize();
    }

    DoubleInterval(const DoubleInterval&) = default;
    DoubleInterval(DoubleInterval&&) = default;
    DoubleInterval& operator=(const DoubleInterval& other) {
        if (this != &other) {
            this->first_ = other.first_;
            this->second_ = other.second_;
        }
        return *this;
    }
    DoubleInterval& operator=(DoubleInterval&&) = default;
    ~DoubleInterval() override = default;

    static Ptr top() { return std::make_shared<Self>(TopTag{}); }
    static Ptr bottom() { return std::make_shared<Self>(BottomTag{}); }
    static Ptr constant(int constant) { return std::make_shared<Self>(constant); }
    static Ptr constant(long constant) { return std::make_shared<Self>(constant); }
    static Ptr constant(double constant) { return std::make_shared<Self>(N1(constant)); }
    static Ptr constant(size_t constant) { return std::make_shared<Self>(N1(constant)); }
    static Ptr constant(const N1& n) { return std::make_shared<Self>(n); }

    static Self getTop() { return Self(TopTag{}); }
    static Self getBottom() { return Self(BottomTag{}); }

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override {
        return first_->isTop() and second_->isTop();
    }

    bool isBottom() const override {
        return first_->isBottom() and second_->isBottom();
    }

    void setTop() override {
        first_->setTop();
        second_->setTop();
    }

    void setBottom() override {
        first_->setBottom();
        second_->setBottom();
    }

    Ptr first() const { return first_; }

    Ptr second() const { return second_; }

    bool leq(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else {
            return this->first_->leq(otherRaw->first_) && this->second_->leq(otherRaw->second_);
        }
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isBottom()) {
            return false;
        } else {
            return this->first_ == otherRaw->first_ && this->second_ == otherRaw->second_;
        }
    }

    Ptr join(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return std::make_shared<Self>(*otherRaw);
        } else if (other->isBottom()) {
            return std::make_shared<Self>(*this);
        } else {
            return std::make_shared<Self>(this->first_->join(otherRaw->first_), this->second_->join(otherRaw->second_));
        }
    }

    void joinWith(ConstPtr other) override {
        this->operator=(*unwrap(join(other)));
    }

    Ptr meet(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || otherRaw->isBottom()) {
            return bottom();
        } else {
            return std::make_shared<Self>(this->first_->meet(otherRaw->first_), this->second_->meet(otherRaw->second_));
        }
    }

    void meetWith(ConstPtr other) override {
        this->operator=(*unwrap(meet(other)));
    }

    Ptr widen(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return std::make_shared<Self>(*otherRaw);
        } else if (other->isBottom()) {
            return std::make_shared<Self>(*this);
        } else {
            return std::make_shared<Self>(this->first_->widen(otherRaw->first_), this->second_->widen(otherRaw->second_));
        }
    }

    void widenWith(AbstractDomain::ConstPtr other) override {
        this->operator=(*unwrap(widen(other)));
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(first_, second_);
    }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "[" << first_->toString() << ", " << second_->toString() << "]" << std::endl;
        return ss.str();
    }

    Ptr apply(llvm::ArithType opcode, ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        return std::make_shared<Self>(first_->apply(opcode, otherRaw->first_), second_->apply(opcode, otherRaw->second_));
    }

    Ptr apply(llvm::ConditionType opcode, ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        return first_->apply(opcode, otherRaw->first_)->join(second_->apply(opcode, otherRaw->second_));
    }

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_DOUBLEINTERVAL_HPP

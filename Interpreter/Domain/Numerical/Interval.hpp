//
// Created by abdullin on 1/25/19.
//

#ifndef BOREALIS_INTERVAL_HPP
#define BOREALIS_INTERVAL_HPP

#include "Bound.hpp"
#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Util/algorithm.hpp"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Number>
class Interval : public AbstractDomain {
public:

    using Self = Interval<Number>;
    using BoundT = Bound<Number>;

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

public:
    struct TopTag {};
    struct BottomTag {};

    explicit Interval(TopTag&) : AbstractDomain(class_tag(*this)), lb_(BoundT::minusInfinity()), ub_(BoundT::plusInfinity()) {}
    explicit Interval(BottomTag&) : AbstractDomain(class_tag(*this)), lb_(1), ub_(0) {}
    explicit Interval(TopTag&&) : AbstractDomain(class_tag(*this)), lb_(BoundT::minusInfinity()), ub_(BoundT::plusInfinity()) {}
    explicit Interval(BottomTag&&) : AbstractDomain(class_tag(*this)), lb_(1), ub_(0) {}

    Interval() : Interval(TopTag{}) {}
    explicit Interval(int n) : AbstractDomain(class_tag(*this)), lb_(n), ub_(n) {}
    explicit Interval(const Number& n) : AbstractDomain(class_tag(*this)), lb_(n), ub_(n) {}
    explicit Interval(const BoundT& b) : AbstractDomain(class_tag(*this)), lb_(b), ub_(b) {
        UNREACHABLE(!b.is_infinite());
    }

    Interval(int from, int to) : AbstractDomain(class_tag(*this)), lb_(from), ub_(to) {}
    Interval(const Number& from, const Number& to) : AbstractDomain(class_tag(*this)), lb_(from), ub_(to) {}
    Interval(const BoundT& from, const BoundT& to) : AbstractDomain(class_tag(*this)), lb_(from), ub_(to) {}

    Interval(const Interval&) = default;
    Interval(Interval&&) = default;
    Interval& operator=(const Interval& other) {
        if (this != &other) {
            this->lb_ = other.lb_;
            this->ub_ = other.ub_;
        }
        return *this;
    }
    Interval& operator=(Interval&&) = default;
    ~Interval() override = default;

    static Ptr top() { return std::make_shared<Self>(TopTag{}); }
    static Ptr bottom() { return std::make_shared<Self>(BottomTag{}); }
    static Ptr constant(int constant) { return std::make_shared<Self>(constant); }
    static Ptr constant(long constant) { return std::make_shared<Self>(constant); }
    static Ptr constant(double constant) { return std::make_shared<Self>(Number(constant)); }
    static Ptr constant(size_t constant) { return std::make_shared<Self>(Number(constant)); }
    static Ptr constant(const Number& n) { return std::make_shared<Self>(n); }

    static Self getTop() { return Self(TopTag{}); }
    static Self getBottom() { return Self(BottomTag{}); }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override {
        return lb_ == BoundT::minusInfinity() and ub_ == BoundT::plusInfinity();
    }

    bool isBottom() const override {
        return lb_ > ub_;
    }

    void setTop() override {
        this->lb_ = BoundT::minusInfinity();
        this->ub_ = BoundT::plusInfinity();
    }

    void setBottom() override {
        this->lb_ = 1;
        this->ub_ = 0;
    }

    Number asConstant() const {
        ASSERT(isConstant(), "Trying to get constant value of non-const interval")
        return lb_.number();
    }

    bool isConstant() const { return lb_ == ub_; }

    bool isConstant(int n) const { return isConstant(BoundT(n)); }

    bool isConstant(const Number& n) const { return isConstant(BoundT(n)); }

    bool isConstant(const BoundT& bnd) const {
        ASSERT(bnd.isFinite(), "")
        return lb_ == bnd and ub_ == bnd;
    }

    bool contains(int n) const { return contains(Number(n)); }

    bool contains(const Number& n) const { return contains(BoundT(n)); }

    bool contains(const BoundT& bnd) const {
        ASSERT(bnd.isFinite(), "")
        return lb_ <= bnd and bnd <= ub_;
    }

    bool intersects(ConstPtr other) const {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        ASSERTC(otherRaw);

        return this->ub_ <= otherRaw->lb_ || otherRaw->ub_ <= this->lb_;
    }

    bool isZero() const { return isConstant(0); }

    const BoundT& lb() const { return lb_; }

    const BoundT& ub() const { return ub_; }

    bool leq(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else {
            return otherRaw->lb_ <= this->lb_ && this->ub_ <= otherRaw->ub_;
        }
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isBottom()) {
            return false;
        } else {
            return this->lb_ == otherRaw->lb_ && this->ub_ == otherRaw->ub_;
        }
    }

    Ptr join(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return std::make_shared<Self>(*otherRaw);
        } else if (other->isBottom()) {
            return std::make_shared<Self>(*this);
        } else {
            return std::make_shared<Self>(util::min(this->lb_, otherRaw->lb_), util::max(this->ub_, otherRaw->ub_));
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
            return std::make_shared<Self>(util::max(this->lb_, otherRaw->lb_), util::min(this->ub_, otherRaw->ub_));
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
            return std::make_shared<Self>(
                    otherRaw->lb_ < this->lb_ ? BoundT::minusInfinity() : lb_,
                    otherRaw->ub_ > this->ub_ ? BoundT::plusInfinity() : ub_
            );
        }
    }

    void widenWith(AbstractDomain::ConstPtr other) override {
        this->operator=(*unwrap(widen(other)));
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(lb_, ub_);
    }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "[" << lb_ << ", " << ub_ << std::endl;
        return ss.str();
    }

    Ptr apply(BinaryOperator opcode, ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        switch (opcode) {
            case ADD: return (*this + *otherRaw).shared_from_this();
            case SUB: return (*this - *otherRaw).shared_from_this();
            case MUL: return (*this * *otherRaw).shared_from_this();
            case DIV: return (*this / *otherRaw).shared_from_this();
            case MOD: return (*this % *otherRaw).shared_from_this();
            case SHL: return (*this << *otherRaw).shared_from_this();
            case SHR: return (*this >> *otherRaw).shared_from_this();
            case LSHR: return lshr(*this, *otherRaw).shared_from_this();
            case AND: return (*this & *otherRaw).shared_from_this();
            case OR: return (*this | *otherRaw).shared_from_this();
            case XOR: return (*this ^ *otherRaw).shared_from_this();
            default:
                UNREACHABLE("Unknown binary operator");
        }
    }

    Ptr operator-() const {
        if (this->isBottom()) {
            return bottom();
        } else {
            return std::make_shared<Self>(-this->_ub, -this->_lb);
        }
    }

    void operator+=(const Self& other) {
        if (this->isBottom()) {
            return;
        } else if (other.isBottom()) {
            this->setBottom();
        } else {
            this->lb_ += other.lb_;
            this->ub_ += other.ub_;
        }
    }

    void operator-=(const Self& other) {
        if (this->isBottom()) {
            return;
        } else if (other.isBottom()) {
            this->setBottom();
        } else {
            this->lb_ -= other.ub_;
            this->ub_ -= other.lb_;
        }
    }

    void operator*=(const Self& other) {
        if (this->isBottom()) {
            return;
        } else if (other.isBottom()) {
            this->setBottom();
        } else {
            auto ll = this->lb_ * other.lb_;
            auto ul = this->ub_ * other.lb_;
            auto lu = this->lb_ * other.ub_;
            auto uu = this->ub_ * other.ub_;
            this->lb_ = util::min(ll, ul, lu, uu);
            this->ub_ = util::max(ll, ul, lu, uu);
        }
    }

private:

    BoundT lb_;
    BoundT ub_;

};

template <typename Number>
Interval<Number> unwrapInterval(AbstractDomain::Ptr interval) {
    return std::move(*llvm::cast<Interval<Number>>(interval.get()));
}

template <typename Number>
Interval<Number> operator+(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {
        return IntervalT(lhv.lb() + rhv.lb(), lhv.ub() + rhv.ub());
    }
}

template <typename Number>
Interval<Number> operator-(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {
        return IntervalT(lhv.lb() - rhv.ub(), lhv.ub() - rhv.lb());
    }
}

template <typename Number>
Interval<Number> operator*(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {
        auto ll = lhv.lb() * rhv.lb();
        auto ul = lhv.ub() * rhv.lb();
        auto lu = lhv.lb() * rhv.ub();
        auto uu = lhv.ub() * rhv.ub();
        return IntervalT(util::min(ll, ul, lu, uu), util::max(ll, ul, lu, uu));
    }
}

template <typename Number>
Interval<Number> operator/(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {
        if (rhv.contains(0)) {
            auto l = IntervalT(rhv.lb(), BoundT(-1));
            auto r = IntervalT(BoundT(1), rhv.ub());
            return *llvm::cast<IntervalT>((lhv / l).join((lhv / r).shared_from_this()));
        } else if (lhv.contains(0)) {
            auto l = IntervalT(lhv.lb(), BoundT(-1));
            auto r = IntervalT(BoundT(1), lhv.ub());
            return unwrapInterval<Number>((l / rhv).join((r / rhv).shared_from_this())->join(IntervalT::constant(0)));
        } else {
            auto ll = lhv.lb() / rhv.lb();
            auto ul = lhv.ub() / rhv.lb();
            auto lu = lhv.lb() / rhv.ub();
            auto uu = lhv.ub() / rhv.ub();
            return IntervalT(util::min(ll, ul, lu, uu), util::max(ll, ul, lu, uu));
        }
    }
}

template <typename Number>
Interval<Number> operator%(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {
        if (rhv.isConstant(0)) {
            return IntervalT::getBottom();
        } else if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() % rhv.asConstant());
        } else {
            BoundT zero(0);
            BoundT n_ub = util::max(abs(lhv.lb()), abs(lhv.ub()));
            BoundT d_ub = util::max(abs(rhv.lb()), abs(rhv.ub())) - BoundT(1);
            BoundT ub = util::min(n_ub, d_ub);

            if (lhv.lb() < zero) {
                if (lhv.ub() > zero) {
                    return IntervalT(-ub, ub);
                } else {
                    return IntervalT(-ub, zero);
                }
            } else {
                return IntervalT(zero, ub);
            }
        }
    }
}

template <typename Number>
Interval<Number> operator<<(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {
        auto&& shift = rhv.meet(std::make_shared<IntervalT>(BoundT(0), BoundT::plusInfinity()));

        if (shift->isBottom()) {
            return IntervalT::getBottom();
        }

        auto* shiftRaw = llvm::cast<IntervalT>(shift.get());

        auto coeff = IntervalT(
                BoundT(1) << shiftRaw->lb(),
                shiftRaw->ub().isFinite() ? (BoundT(1) << shiftRaw->ub()) : BoundT::plusInfinity()
        );
        return lhv * coeff;
    }
}

template <typename Number>
Interval<Number> operator>>(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {
        auto&& shift = rhv.meet(std::make_shared<IntervalT>(BoundT(0), BoundT::plusInfinity()));
        auto* shiftRaw = llvm::cast<IntervalT>(shift.get());

        if (shift->isBottom()) {
            return IntervalT::getBottom();
        }

        if (lhv.contains(0)) {
            auto l = IntervalT(lhv.lb(), BoundT(-1));
            auto u = IntervalT(BoundT(1), lhv.ub());
            return unwrapInterval<Number>((l >> rhv).join((u >> rhv).shared_from_this())->join(IntervalT::constant(0)));
        } else {
            auto ll = lhv.lb() >> shiftRaw->lb();
            auto lu = lhv.lb() >> shiftRaw->ub();
            auto ul = lhv.ub() >> shiftRaw->lb();
            auto uu = lhv.ub() >> shiftRaw->ub();
            return IntervalT(util::min(ll, lu, ul, uu), util::max(ll, lu, ul, uu));
        }
    }
}

template <typename Number>
Interval<Number> lshr(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {
        auto&& shift = rhv.meet(std::make_shared<IntervalT>(BoundT(0), BoundT::plusInfinity()));
        auto* shiftRaw = llvm::cast<IntervalT>(shift.get());

        if (shift->isBottom()) {
            return IntervalT::getBottom();
        }

        if (lhv.contains(0)) {
            auto l = IntervalT(lhv.lb(), BoundT(-1));
            auto u = IntervalT(BoundT(1), lhv.ub());
            return unwrapInterval<Number>(lshr(l, rhv).join(lshr(u, rhv).shared_from_this())->join(IntervalT::constant(0)));
        } else {
            BoundT ll = lshr(lhv.lb(), shiftRaw->lb());
            BoundT lu = lshr(lhv.lb(), shiftRaw->ub());
            BoundT ul = lshr(lhv.ub(), shiftRaw->lb());
            BoundT uu = lshr(lhv.ub(), shiftRaw->ub());
            return IntervalT(util::min(ll, lu, ul, uu), util::max(ll, lu, ul, uu));
        }
    }
}

template <typename Number>
Interval<Number> operator&(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {

        if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() & rhv.asConstant());
        } else if (lhv.isConstant(0) || rhv.isConstant(0)) {
            return IntervalT(0);
        } else if (lhv.isConstant(-1)) {
            return rhv;
        } else if (rhv.isConstant(-1)) {
            return lhv;
        } else {
            if (lhv.lb() >= BoundT(0) && rhv.lb() >= BoundT(0)) {
                return IntervalT(BoundT(0), util::min(lhv.ub(), rhv.ub()));
            } else if (lhv.lb() >= BoundT(0)) {
                return IntervalT(BoundT(0), lhv.ub());
            } else if (lhv.lb() >= BoundT(0)) {
                return IntervalT(BoundT(0), rhv.ub());
            } else {
                return IntervalT::getTop();
            }
        }
    }
}

template <typename Number>
Interval<Number> operator|(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {

        if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() | rhv.asConstant());
        } else if (lhv.isConstant(-1) || rhv.isConstant(-1)) {
            return IntervalT(-1);
        } else if (rhv.isConstant(0)) {
            return lhv;
        } else if (lhv.isConstant(0)) {
            return rhv;
        } else {
            return IntervalT::getTop();
        }
    }
}

template <typename Number>
Interval<Number> operator^(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom();
    } else {

        if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() ^ rhv.asConstant());
        } else if (rhv.isConstant(0)) {
            return lhv;
        } else if (lhv.isConstant(0)) {
            return rhv;
        } else {
            return IntervalT::getTop();
        }
    }
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"


#endif //BOREALIS_INTERVAL_HPP

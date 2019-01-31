//
// Created by abdullin on 1/25/19.
//

#ifndef BOREALIS_INTERVAL_HPP
#define BOREALIS_INTERVAL_HPP

#include "Bound.hpp"
#include "Interpreter/Domain/Domain.h"
#include "Util/algorithm.hpp"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Number>
class Interval : public AbstractDomain<Interval<Number>> {
public:

    using BoundT = Bound<Number>;

private:
    struct TopTag {};
    struct BottomTag {};

    explicit Interval(TopTag) : lb_(BoundT::minusInfinity()), ub_(BoundT::plusInfinity()) {}

    explicit Interval(BottomTag) : lb_(1), ub_(0) {}

public:
    Interval() : Interval(TopTag{}) {}

    explicit Interval(int n) : lb_(n), ub_(n) {}

    explicit Interval(const Number& n) : lb_(n), ub_(n) {}

    explicit Interval(const BoundT& b) : lb_(b), ub_(b) {
        UNREACHABLE(!b.is_infinite());
    }

    Interval(const Interval&) = default;

    Interval(Interval&&) = default;

    Interval& operator=(const Interval&) = default;

    Interval& operator=(Interval&&) = default;

    ~Interval() override = default;

    static Interval top() { return Interval(TopTag{}); }

    static Interval bottom() { return Interval(BottomTag{}); }

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

    bool isConstant(const BoundT& bnd) {
        ASSERT(bnd.isFinite(), "")
        return lb_ == bnd and ub_ == bnd;
    }

    bool contains(int n) const { return contains(Number(n)); }

    bool contains(const Number& n) const { return contains(BoundT(n)); }

    bool contains(const BoundT& bnd) {
        ASSERT(bnd.isFinite(), "")
        return lb_ <= bnd and bnd <= ub_;
    }

    bool intersects(const Interval& other) {
        return this->ub_ <= other.lb_ || other.ub_ <= this->lb_;
    }

    bool isZero() const { return isConstant(0); }

    const BoundT& lb() const { return lb_; }

    const BoundT& ub() const { return ub_; }

    bool leq(const Interval& other) const override {
        if (this->isBottom()) {
            return true;
        } else if (other.isBottom()) {
            return false;
        } else {
            return other.lb_ <= this->lb_ && this->ub_ <= other.ub_;
        }
    }

    bool equals(const Interval& other) const override {
        if (this->isBottom()) {
            return other.isBottom();
        } else if (other.isBottom()) {
            return false;
        } else {
            return this->lb_ == other.lb_ && this->ub_ == other.ub_;
        }
    }

    Interval join(const Interval& other) const override {
        if (this->isBottom()) {
            return other;
        } else if (other.isBottom()) {
            return *this;
        } else {
            return Interval(util::min(this->lb_, other.lb_), util::max(this->ub_, other.ub_));
        }
    }

    void joinWith(const Interval& other) override {
        this->operator=(std::move(join(other)));
    }

    Interval meet(const Interval& other) const override {
        if (this->isBottom() || other.isBottom()) {
            return bottom();
        } else {
            return Interval(util::max(this->lb_, other.lb_), util::min(this->ub_, other.ub_));
        }
    }

    void meetWith(const Interval& other) override {
        this->operator=(std::move(meet(other)));
    }

    Interval widen(const Interval& other) const override {
        if (this->isBottom()) {
            return other;
        } else if (other.isBottom()) {
            return *this;
        } else {
            return Interval(
                    other.lb_ < this->lb_ ? BoundT::minusInfinity() : lb_,
                    other.ub_ > this->ub_ ? BoundT::plusInfinity() : ub_
            );
        }
    }

    void widenWith(const Interval& other) override {
        this->operator=(std::move(widen(other)));
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(lb_, ub_);
    }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "[" << lb_ << ", " << ub_ << std::endl;
        return ss.str();
    }

    Interval apply(BinaryOperator opcode, const Interval& other) {
        switch (opcode) {
            case ADD: return *this + other;
            case SUB: return *this - other;
            case MUL: return *this * other;
            case DIV: return *this / other;
            case MOD: return *this % other;
            case SHL: return *this << other;
            case SHR: return *this >> other;
            case LSHR: return lshr(*this, other);
            case AND: return *this & other;
            case OR: return *this | other;
            case XOR: return *this ^ other;
            default:
                UNREACHABLE("Unknown binary operator");
        }
    }

    Interval operator-() const {
        if (this->is_bottom()) {
            return bottom();
        } else {
            return Interval(-this->_ub, -this->_lb);
        }
    }

    void operator+=(const Interval& other) {
        if (this->isBottom()) {
            return;
        } else if (other.isBottom()) {
            this->setBottom();
        } else {
            this->lb_ += other.lb_;
            this->ub_ += other.ub_;
        }
    }

    void operator-=(const Interval& other) {
        if (this->isBottom()) {
            return;
        } else if (other.isBottom()) {
            this->setBottom();
        } else {
            this->lb_ -= other.ub_;
            this->ub_ -= other.lb_;
        }
    }

    void operator*=(const Interval& other) {
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
Interval<Number> operator+(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::bottom();
    } else {
        return IntervalT(lhv.lb() + rhv.lb(), lhv.ub() + rhv.ub());
    }
}

template <typename Number>
Interval<Number> operator-(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::bottom();
    } else {
        return IntervalT(lhv.lb() - rhv.ub(), lhv.ub() - rhv.lb());
    }
}

template <typename Number>
Interval<Number> operator*(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::bottom();
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
        return IntervalT::bottom();
    } else {
        if (rhv.contains(0)) {
            IntervalT l(rhv.lb(), BoundT(-1));
            IntervalT r(BoundT(1), rhv.ub());
            return (lhv / l).join(lhv / r);
        } else if (lhv.contains(0)) {
            IntervalT l(lhv.lb(), BoundT(-1));
            IntervalT r(BoundT(1), lhv.ub());
            return (l / rhv).join(r / rhv).join(IntervalT(0));
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
        return IntervalT::bottom();
    } else {
        if (rhv.isConstant(0)) {
            return IntervalT::bottom();
        } else if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() % rhv.asConstant());
        } else {
            BoundT zero(0);
            BoundT n_ub = max(abs(lhv.lb()), abs(lhv.ub()));
            BoundT d_ub = max(abs(rhv.lb()), abs(rhv.ub())) - BoundT(1);
            BoundT ub = min(n_ub, d_ub);

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
        return IntervalT::bottom();
    } else {
        IntervalT shift = rhv.meet(IntervalT(BoundT(0), BoundT::plus_infinity()));

        if (shift.isBottom()) {
            return IntervalT::bottom();
        }

        IntervalT coeff(BoundT(1 << *shift.lb().number()),
                        shift.ub().is_finite() ? BoundT(1 << *shift.ub().number())
                                               : BoundT::plus_infinity());
        return lhv * coeff;
    }
}

template <typename Number>
Interval<Number> operator>>(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::bottom();
    } else {
        IntervalT shift = rhv.meet(IntervalT(BoundT(0), BoundT::plus_infinity()));

        if (shift.is_bottom()) {
            return IntervalT::bottom();
        }

        if (lhv.contains(0)) {
            IntervalT l(lhv.lb(), BoundT(-1));
            IntervalT u(BoundT(1), lhv.ub());
            return (l >> rhv).join(u >> rhv).join(IntervalT(0));
        } else {
            auto ll = lhv.lb() >> shift.lb();
            auto lu = lhv.lb() >> shift.ub();
            auto ul = lhv.ub() >> shift.lb();
            auto uu = lhv.ub() >> shift.ub();
            return IntervalT(min(ll, lu, ul, uu), max(ll, lu, ul, uu));
        }
    }
}

template <typename Number>
Interval<Number> lshr(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::bottom();
    } else {
        IntervalT shift = rhv.meet(IntervalT(BoundT(0), BoundT::plus_infinity()));

        if (shift.is_bottom()) {
            return IntervalT::bottom();
        }

        if (lhv.contains(0)) {
            IntervalT l(lhv.lb(), BoundT(-1));
            IntervalT u(BoundT(1), lhv.ub());
            return lshr(l, rhv).join(lshr(u, rhv)).join(IntervalT(0));
        } else {
            BoundT ll = lshr(lhv.lb(), shift.lb());
            BoundT lu = lshr(lhv.lb(), shift.ub());
            BoundT ul = lshr(lhv.ub(), shift.lb());
            BoundT uu = lshr(lhv.ub(), shift.ub());
            return IntervalT(min(ll, lu, ul, uu), max(ll, lu, ul, uu));
        }
    }
}

template <typename Number>
Interval<Number> operator&(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::bottom();
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
                return IntervalT(BoundT(0), min(lhv.ub(), rhv.ub()));
            } else if (lhv.lb() >= BoundT(0)) {
                return IntervalT(BoundT(0), lhv.ub());
            } else if (lhv.lb() >= BoundT(0)) {
                return IntervalT(BoundT(0), rhv.ub());
            } else {
                return IntervalT::top();
            }
        }
    }
}

template <typename Number>
Interval<Number> operator|(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::bottom();
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
            return IntervalT::top();
        }
    }
}

template <typename Number>
Interval<Number> operator^(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::bottom();
    } else {

        if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() ^ rhv.asConstant());
        } else if (rhv.isConstant(0)) {
            return lhv;
        } else if (lhv.isConstant(0)) {
            return rhv;
        } else {
            return IntervalT::top();
        }
    }
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"


#endif //BOREALIS_INTERVAL_HPP

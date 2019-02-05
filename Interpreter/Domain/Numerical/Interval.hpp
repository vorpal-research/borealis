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
class Interval : public AbstractDomain {
public:

    using Self = Interval<Number>;
    using BoundT = Bound<Number>;

private:
    struct TopTag {};
    struct BottomTag {};

    explicit Interval(TopTag) : AbstractDomain(class_tag(*this)), lb_(BoundT::minusInfinity()), ub_(BoundT::plusInfinity()) {}
    explicit Interval(BottomTag) : AbstractDomain(class_tag(*this)), lb_(1), ub_(0) {}

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

    static Ptr top() { return std::make_shared(TopTag{}); }
    static Ptr bottom() { return std::make_shared(BottomTag{}); }
    static Ptr constant(int constant) { return std::make_shared<Interval<Number>>(constant); }

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

    bool intersects(ConstPtr other) {
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
            return otherRaw->shared_from_this();
        } else if (other->isBottom()) {
            return *this;
        } else {
            return std::make_shared(util::min(this->lb_, otherRaw->lb_), util::max(this->ub_, otherRaw->ub_));
        }
    }

    void joinWith(ConstPtr other) override {
        this->operator=(std::move(join(other)));
    }

    Ptr meet(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || otherRaw->isBottom()) {
            return bottom();
        } else {
            return std::make_shared(util::max(this->lb_, otherRaw->lb_), util::min(this->ub_, otherRaw->ub_));
        }
    }

    void meetWith(ConstPtr other) override {
        this->operator=(std::move(meet(other)));
    }

    Ptr widen(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return otherRaw->shared_from_this();
        } else if (other->isBottom()) {
            return *this;
        } else {
            return std::make_shared(
                    otherRaw->lb_ < this->lb_ ? BoundT::minusInfinity() : lb_,
                    otherRaw->ub_ > this->ub_ ? BoundT::plusInfinity() : ub_
            );
        }
    }

    void widenWith(ConstPtr other) override {
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

    Ptr apply(BinaryOperator opcode, ConstPtr other) {
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

    Ptr operator-() const {
        if (this->isBottom()) {
            return bottom();
        } else {
            return std::make_shared(-this->_ub, -this->_lb);
        }
    }

    void operator+=(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (other->isBottom()) {
            this->setBottom();
        } else {
            this->lb_ += otherRaw->lb_;
            this->ub_ += otherRaw->ub_;
        }
    }

    void operator-=(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (other->isBottom()) {
            this->setBottom();
        } else {
            this->lb_ -= otherRaw->ub_;
            this->ub_ -= otherRaw->lb_;
        }
    }

    void operator*=(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (other->isBottom()) {
            this->setBottom();
        } else {
            auto ll = this->lb_ * otherRaw->lb_;
            auto ul = this->ub_ * otherRaw->lb_;
            auto lu = this->lb_ * otherRaw->ub_;
            auto uu = this->ub_ * otherRaw->ub_;
            this->lb_ = util::min(ll, ul, lu, uu);
            this->ub_ = util::max(ll, ul, lu, uu);
        }
    }

private:

    BoundT lb_;
    BoundT ub_;

};

template <typename Number>
using IntervalPtr = typename Interval<Number>::Ptr;

template <typename Number>
using IntervalConstPtr = typename Interval<Number>::ConstPtr;

template <typename Number>
IntervalPtr<Number> operator+(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {
        return std::make_shared(lhv->lb() + rhv->lb(), lhv->ub() + rhv->ub());
    }
}

template <typename Number>
IntervalPtr<Number> operator-(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {
        return std::make_shared(lhv->lb() - rhv->ub(), lhv->ub() - rhv->lb());
    }
}

template <typename Number>
IntervalPtr<Number> operator*(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {
        auto ll = lhv->lb() * rhv->lb();
        auto ul = lhv->ub() * rhv->lb();
        auto lu = lhv->lb() * rhv->ub();
        auto uu = lhv->ub() * rhv->ub();
        return std::make_shared(util::min(ll, ul, lu, uu), util::max(ll, ul, lu, uu));
    }
}

template <typename Number>
IntervalPtr<Number> operator/(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {
        if (rhv->contains(0)) {
            IntervalPtr<Number> l = std::make_shared(rhv->lb(), BoundT(-1));
            IntervalPtr<Number> r = std::make_shared(BoundT(1), rhv->ub());
            return (lhv / l)->join(lhv / r);
        } else if (lhv->contains(0)) {
            IntervalPtr<Number> l = std::make_shared(lhv->lb(), BoundT(-1));
            IntervalPtr<Number> r = std::make_shared(BoundT(1), lhv->ub());
            return (l / rhv)->join(r / rhv)->join(IntervalT::contains(0));
        } else {
            auto ll = lhv->lb() / rhv->lb();
            auto ul = lhv->ub() / rhv->lb();
            auto lu = lhv->lb() / rhv->ub();
            auto uu = lhv->ub() / rhv->ub();
            return std::make_shared(util::min(ll, ul, lu, uu), util::max(ll, ul, lu, uu));
        }
    }
}

template <typename Number>
IntervalPtr<Number> operator%(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {
        if (rhv->isConstant(0)) {
            return IntervalT::bottom();
        } else if (lhv->isConstant() && rhv->isConstant()) {
            return std::make_shared(lhv->asConstant() % rhv->asConstant());
        } else {
            BoundT zero(0);
            BoundT n_ub = max(abs(lhv->lb()), abs(lhv->ub()));
            BoundT d_ub = max(abs(rhv->lb()), abs(rhv->ub())) - BoundT(1);
            BoundT ub = min(n_ub, d_ub);

            if (lhv->lb() < zero) {
                if (lhv->ub() > zero) {
                    return std::make_shared(-ub, ub);
                } else {
                    return std::make_shared(-ub, zero);
                }
            } else {
                return std::make_shared(zero, ub);
            }
        }
    }
}

template <typename Number>
IntervalPtr<Number> operator<<(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {
        auto&& shift = rhv->meet(std::make_shared(BoundT(0), BoundT::plus_infinity()));

        if (shift->isBottom()) {
            return IntervalT::bottom();
        }

        IntervalPtr<Number> coeff = std::make_shared(
                BoundT(1 << *shift->lb()->number()),
                shift->ub()->is_finite() ? BoundT(1 << *shift->ub()->number()) : BoundT::plus_infinity()
        );
        return lhv * coeff;
    }
}

template <typename Number>
IntervalPtr<Number> operator>>(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {
        auto&& shift = rhv->meet(std::make_shared(BoundT(0), BoundT::plus_infinity()));

        if (shift->is_bottom()) {
            return IntervalT::bottom();
        }

        if (lhv->contains(0)) {
            IntervalPtr<Number> l = std::make_shared(lhv->lb(), BoundT(-1));
            IntervalPtr<Number> u = std::make_shared(BoundT(1), lhv->ub());
            return (l >> rhv)->join(u >> rhv)->join(IntervalT::constant(0));
        } else {
            auto ll = lhv->lb() >> shift->lb();
            auto lu = lhv->lb() >> shift->ub();
            auto ul = lhv->ub() >> shift->lb();
            auto uu = lhv->ub() >> shift->ub();
            return std::make_shared(min(ll, lu, ul, uu), max(ll, lu, ul, uu));
        }
    }
}

template <typename Number>
IntervalPtr<Number> lshr(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {
        auto&& shift = rhv->meet(std::make_shared(BoundT(0), BoundT::plus_infinity()));

        if (shift->is_bottom()) {
            return IntervalT::bottom();
        }

        if (lhv->contains(0)) {
            IntervalPtr<Number> l = std::make_shared(lhv->lb(), BoundT(-1));
            IntervalPtr<Number> u = std::make_shared(BoundT(1), lhv->ub());
            return lshr(l, rhv)->join(lshr(u, rhv))->join(IntervalT::constant(0));
        } else {
            BoundT ll = lshr(lhv->lb(), shift->lb());
            BoundT lu = lshr(lhv->lb(), shift->ub());
            BoundT ul = lshr(lhv->ub(), shift->lb());
            BoundT uu = lshr(lhv->ub(), shift->ub());
            return std::make_shared(min(ll, lu, ul, uu), max(ll, lu, ul, uu));
        }
    }
}

template <typename Number>
IntervalPtr<Number> operator&(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {

        if (lhv->isConstant() && rhv->isConstant()) {
            return std::make_shared(lhv->asConstant() & rhv->asConstant());
        } else if (lhv->isConstant(0) || rhv->isConstant(0)) {
            return IntervalT::constant(0);
        } else if (lhv->isConstant(-1)) {
            return rhv;
        } else if (rhv->isConstant(-1)) {
            return lhv;
        } else {
            if (lhv->lb() >= BoundT(0) && rhv->lb() >= BoundT(0)) {
                return std::make_shared(BoundT(0), min(lhv->ub(), rhv->ub()));
            } else if (lhv->lb() >= BoundT(0)) {
                return std::make_shared(BoundT(0), lhv->ub());
            } else if (lhv->lb() >= BoundT(0)) {
                return std::make_shared(BoundT(0), rhv->ub());
            } else {
                return IntervalT::top();
            }
        }
    }
}

template <typename Number>
IntervalPtr<Number> operator|(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {

        if (lhv->isConstant() && rhv->isConstant()) {
            return std::make_shared(lhv->asConstant() | rhv->asConstant());
        } else if (lhv->isConstant(-1) || rhv->isConstant(-1)) {
            return IntervalT::constant(-1);
        } else if (rhv->isConstant(0)) {
            return lhv;
        } else if (lhv->isConstant(0)) {
            return rhv;
        } else {
            return IntervalT::top();
        }
    }
}

template <typename Number>
IntervalPtr<Number> operator^(IntervalConstPtr<Number> lhv, IntervalConstPtr<Number> rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv->isBottom() || rhv->isBottom()) {
        return IntervalT::bottom();
    } else {

        if (lhv->isConstant() && rhv->isConstant()) {
            return std::make_shared(lhv->asConstant() ^ rhv->asConstant());
        } else if (rhv->isConstant(0)) {
            return lhv;
        } else if (lhv->isConstant(0)) {
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

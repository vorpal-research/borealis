//
// Created by abdullin on 1/25/19.
//

#ifndef BOREALIS_INTERVAL_HPP
#define BOREALIS_INTERVAL_HPP

#include "Bound.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Util/algorithm.hpp"
#include "Util/cache.hpp"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

namespace impl_ {
template <typename Num>
using IntervalID = std::pair<Bound<Num>, Bound<Num>>;

template <typename N>
struct IntervalIDHash {
    std::size_t operator()(const IntervalID<N>& id) const {
        return util::hash::defaultHasher()(id.first, id.second);
    }
};

template <typename N>
struct IntervalIDEquals {
    bool operator()(const IntervalID<N>& lhv, const IntervalID<N>& rhv) const {
        if (lhv.first.caster()->width() != rhv.first.caster()->width()) return false;
        return lhv.first == rhv.first && lhv.second == rhv.second;
    }
};

} // namespace impl_

template <typename Number>
class Interval : public AbstractDomain {
private:

    using IntervalCacheImpl =
            std::unordered_map<impl_::IntervalID<Number>, Ptr, impl_::IntervalIDHash<Number>, impl_::IntervalIDEquals<Number>>;

private:

    static util::cacheWImpl<impl_::IntervalID<Number>, Ptr, IntervalCacheImpl> cache_;

public:

    using Self = Interval<Number>;
    using BoundT = Bound<Number>;
    using CasterT = typename BoundT::CasterT;

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

    static Ptr clone(const Interval<Number>& i) {
        return interval(i.lb(), i.ub());
    }

private:

    const BoundT lb_;
    const BoundT ub_;
    const bool isBottom_;
    const bool isTop_;

public:
    struct TopTag {};
    struct BottomTag {};

    Interval(TopTag, const CasterT* caster) : AbstractDomain(class_tag(*this)), lb_(BoundT::minusInfinity(caster)), ub_(BoundT::plusInfinity(caster)), isBottom_(false), isTop_(true) {}
    Interval(BottomTag, const CasterT* caster) : AbstractDomain(class_tag(*this)), lb_(caster, 1), ub_(caster, 0), isBottom_(true), isTop_(false) {}

    explicit Interval(const CasterT* caster) : Interval(TopTag{}, caster) {}
    Interval(int n, const CasterT* caster) : AbstractDomain(class_tag(*this)), lb_(caster, n), ub_(caster, n), isBottom_(false), isTop_(false) {}
    Interval(const Number& n, const CasterT* caster) : AbstractDomain(class_tag(*this)), lb_(caster, n), ub_(caster, n), isBottom_(false), isTop_(false) {}
    Interval(const BoundT& b) : AbstractDomain(class_tag(*this)), lb_(b), ub_(b), isBottom_(false), isTop_(false) {
        UNREACHABLE(!b.is_infinite());
    }

    Interval(int from, int to, const CasterT* caster) : AbstractDomain(class_tag(*this)), lb_(caster, from), ub_(caster, to) {}
    Interval(const Number& from, const Number& to, const CasterT* caster)
        : AbstractDomain(class_tag(*this)), lb_(caster, from), ub_(caster, to), isBottom_(lb_ > ub_), isTop_(false) {}
    Interval(const BoundT& from, const BoundT& to) : AbstractDomain(class_tag(*this)), lb_(from), ub_(to),
        isBottom_(lb_ > ub_), isTop_(lb_ == BoundT::minusInfinity(lb_.caster()) and ub_ == BoundT::plusInfinity(ub_.caster())) {}

    Interval(const Interval&) = default;
    Interval(Interval&&) = default;
    ~Interval() override = default;

    static Ptr top(const CasterT* caster) { return cache_[std::make_pair(BoundT::minusInfinity(caster), BoundT::plusInfinity(caster))]; }
    static Ptr bottom(const CasterT* caster) { return cache_[std::make_pair(BoundT(caster, 1), BoundT(caster, 0))]; }
    static Ptr constant(int constant, const CasterT* caster) { return cache_[std::make_pair(BoundT(caster, constant), BoundT(caster, constant))]; }
    static Ptr constant(long constant, const CasterT* caster) { return cache_[std::make_pair(BoundT(caster, constant), BoundT(caster, constant))]; }
    static Ptr constant(double constant, const CasterT* caster) { return cache_[std::make_pair(BoundT(caster, constant), BoundT(caster, constant))]; }
    static Ptr constant(size_t constant, const CasterT* caster) { return cache_[std::make_pair(BoundT(caster, constant), BoundT(caster, constant))]; }
    static Ptr constant(const Number& n, const CasterT* caster) { return cache_[std::make_pair(BoundT(caster, n), BoundT(caster, n))]; }
    static Ptr interval(const BoundT& from, const BoundT& to) { return cache_[std::make_pair(from, to)]; }

    static Self getTop(const CasterT* caster) { return Self(TopTag{}, caster); }
    static Self getBottom(const CasterT* caster) { return Self(BottomTag{}, caster); }

    Ptr clone() const override {
        return interval(lb(), ub());
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override {
        return isTop_;
    }

    bool isBottom() const override {
        return isBottom_;
    }

    void setTop() override {
        UNREACHABLE("Should not change interval domains");
    }

    void setBottom() override {
        UNREACHABLE("Should not change interval domains");
    }

    Number asConstant() const {
        ASSERT(isConstant(), "Trying to get constant value of non-const interval")
        return lb_.number();
    }

    const CasterT* caster() const { return lb_.caster(); }

    bool isConstant() const { return lb_ == ub_; }

    bool isConstant(int n) const { return isConstant(BoundT(caster(), n)); }

    bool isConstant(const Number& n) const { return isConstant(BoundT(caster(), n)); }

    bool isConstant(const BoundT& bnd) const {
        ASSERT(bnd.isFinite(), "")
        return lb_ == bnd and ub_ == bnd;
    }

    bool contains(int n) const { return contains(BoundT(caster(), n)); }

    bool contains(const Number& n) const { return contains(BoundT(caster(), n)); }

    bool contains(const BoundT& bnd) const {
        ASSERT(bnd.isFinite(), "")
        return lb_ <= bnd and bnd <= ub_;
    }

    bool intersects(ConstPtr other) const {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        ASSERTC(otherRaw);

        return (this->lb_ <= otherRaw->lb_ && otherRaw->lb_ <= this->ub_) ||
                (otherRaw->lb_ <= this->lb_ && this->lb_ <= otherRaw->ub_);
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
        if (this == other.get()) return true;

        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        if (not otherRaw) return false;

        if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isBottom()) {
            return false;
        } else {
            return this->lb_ == otherRaw->lb_ && this->ub_ == otherRaw->ub_;
        }
    }

    Ptr join(ConstPtr other) const override {
        if (this == other.get())
            return const_cast<Interval<Number>*>(this)->shared_from_this();
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->clone();
        } else if (other->isBottom()) {
            return const_cast<Interval<Number>*>(this)->shared_from_this();
        } else {
            return interval(util::min(this->lb_, otherRaw->lb_), util::max(this->ub_, otherRaw->ub_));
        }
    }

    Ptr meet(ConstPtr other) const override {
        if (this == other.get())
            return const_cast<Interval<Number>*>(this)->shared_from_this();
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || otherRaw->isBottom()) {
            return bottom(caster());
        } else {
            return interval(util::max(this->lb_, otherRaw->lb_), util::min(this->ub_, otherRaw->ub_));
        }
    }

    Ptr widen(ConstPtr other) const override {
        if (this == other.get())
            return const_cast<Interval<Number>*>(this)->shared_from_this();
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->clone();
        } else if (other->isBottom()) {
            return const_cast<Interval<Number>*>(this)->shared_from_this();
        } else {
            return interval(
                    otherRaw->lb_ < this->lb_ ? BoundT::minusInfinity(caster()) : lb_,
                    otherRaw->ub_ > this->ub_ ? BoundT::plusInfinity(caster()) : ub_
            );
        }
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(lb_, ub_);
    }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "[";
        if (this->isBottom()) ss << " BOTTOM ";
        else ss << lb_ << ", " << ub_;
        ss << "]";
        return ss.str();
    }

    Ptr apply(llvm::ArithType opcode, ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        switch (opcode) {
            case llvm::ArithType::ADD: return clone(*this + *otherRaw);
            case llvm::ArithType::SUB: return clone(*this - *otherRaw);
            case llvm::ArithType::MUL: return clone(*this * *otherRaw);
            case llvm::ArithType::DIV: return clone(*this / *otherRaw);
            case llvm::ArithType::REM: return clone(*this % *otherRaw);
            case llvm::ArithType::SHL: return clone(*this << *otherRaw);
            case llvm::ArithType::ASHR: return clone(*this >> *otherRaw);
            case llvm::ArithType::LSHR: return clone(lshr(*this, *otherRaw));
            case llvm::ArithType::BAND: return clone(*this & *otherRaw);
            case llvm::ArithType::LAND: return clone(*this & *otherRaw);
            case llvm::ArithType::BOR: return clone(*this | *otherRaw);
            case llvm::ArithType::LOR: return clone(*this | *otherRaw);
            case llvm::ArithType::XOR: return clone(*this ^ *otherRaw);
            case llvm::ArithType::IMPLIES: return top(caster());
            default:
                UNREACHABLE("Unknown binary operator");
        }
    }

    Ptr operator-() const {
        if (this->isBottom()) {
            return bottom();
        } else {
            return interval(-this->_ub, -this->_lb);
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

    Ptr apply(llvm::ConditionType op, ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        auto* af = AbstractFactory::get();

        auto&& makeTop = [&]() { return af->getBool(AbstractFactory::TOP); };
        auto&& makeBool = [&](bool b) { return af->getBool(b); };

        if (this->isTop() || otherRaw->isTop()) {
            return makeTop();
        } else if (this->isBottom() || otherRaw->isBottom()) {
            return makeTop();//af->getBool(AbstractFactory::BOTTOM);
        }

        switch (op) {
            case llvm::ConditionType::TRUE:
                return makeBool(true);
            case llvm::ConditionType::FALSE:
                return makeBool(false);
            case llvm::ConditionType::UNKNOWN:
                return makeTop();
            case llvm::ConditionType::EQ:
                if (this->isConstant() && otherRaw->isConstant()) {
                    return makeBool(this->asConstant() == otherRaw->asConstant());
                } else if (this->intersects(other)) {
                    return makeTop();
                } else {
                    return makeBool(false);
                }
            case llvm::ConditionType::NEQ:
                if (this->isConstant() && otherRaw->isConstant()) {
                    return makeBool(this->asConstant() != otherRaw->asConstant());
                } else if (this->intersects(other)) {
                    return makeTop();
                } else {
                    return makeBool(true);
                }
            case llvm::ConditionType::ULT:
            case llvm::ConditionType::LT:
                if (this->intersects(other)) {
                    return makeTop();
                } else {
                    return makeBool(this->ub() < otherRaw->lb());
                }
            case llvm::ConditionType::ULE:
            case llvm::ConditionType::LE:
                if (this->intersects(other)) {
                    return makeTop();
                } else {
                    return makeBool(this->ub() <= otherRaw->lb());
                }
            case llvm::ConditionType::UGT:
            case llvm::ConditionType::GT:
                if (this->intersects(other)) {
                    return makeTop();
                } else {
                    return makeBool(otherRaw->ub() < this->lb());
                }
            case llvm::ConditionType::UGE:
            case llvm::ConditionType::GE:
                if (this->intersects(other)) {
                    return makeTop();
                } else {
                    return makeBool(otherRaw->ub() <= this->lb());
                }
            default:
                UNREACHABLE("Unknown cmp operation");
        }
    }

    Split splitByEq(ConstPtr other) const override {
        if (this->isBottom() || other->isBottom()) return { clone(), clone() };
        auto* otherRaw = unwrap(other);

        return otherRaw->isConstant() ?
               Split{ other->clone(), clone() } :
               Split{ clone(), clone() };
    }

    Split splitByLess(ConstPtr other) const override {
        if (this->isBottom() || other->isBottom()) return { clone(), clone() };
        auto* otherRaw = unwrap(other);

        if (this->leq(other)) return { clone(), clone() };
        if (this->ub() < otherRaw->lb()) return { clone(), clone() };
        if (otherRaw->ub() < this->lb()) return { clone(), clone() };

        auto trueVal = interval(lb_, otherRaw->ub_);
        auto falseVal = interval(otherRaw->lb_, this->ub_);
        return { trueVal, falseVal };
    }

};

template <typename Number>
util::cacheWImpl<impl_::IntervalID<Number>, AbstractDomain::Ptr, typename Interval<Number>::IntervalCacheImpl>
        Interval<Number>::cache_([] (const impl_::IntervalID<Number>& a) -> AbstractDomain::Ptr {
            return std::make_shared<Interval<Number>>(a.first, a.second);
        });

template <typename Number>
Interval<Number> operator+(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom(lhv.caster());
    } else {
        return IntervalT(lhv.lb() + rhv.lb(), lhv.ub() + rhv.ub());
    }
}

template <typename Number>
Interval<Number> operator-(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom(lhv.caster());
    } else {
        return IntervalT(lhv.lb() - rhv.ub(), lhv.ub() - rhv.lb());
    }
}

template <typename Number>
Interval<Number> operator*(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom(lhv.caster());
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
        return IntervalT::getBottom(lhv.caster());
    } else {
        if (rhv.contains(0)) {
            return IntervalT::getTop(lhv.caster());
//            auto l = IntervalT(rhv.lb(), BoundT(lhv.caster(), -1));
//            auto r = IntervalT(BoundT(lhv.caster(), 1), rhv.ub());
//            return *llvm::cast<IntervalT>((lhv / l).join(std::make_shared<IntervalT>(lhv / r)));
        } else if (lhv.contains(0)) {
            return IntervalT::getTop(lhv.caster());
//            auto l = IntervalT(lhv.lb(), BoundT(lhv.caster(), -1));
//            auto r = IntervalT(BoundT(lhv.caster(), 1), lhv.ub());
//            return unwrapInterval<Number>((l / rhv).join(std::make_shared<IntervalT>(r / rhv))->join(IntervalT::constant(0, lhv.caster())));
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
        return IntervalT::getBottom(lhv.caster());
    } else {
        if (rhv.isZero()) {
            return IntervalT::getBottom(lhv.caster());
        } else if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() % rhv.asConstant(), lhv.caster());
        } else {
            BoundT zero(lhv.caster(), 0);
            BoundT n_ub = util::max(abs(lhv.lb()), abs(lhv.ub()));
            BoundT d_ub = util::max(abs(rhv.lb()), abs(rhv.ub())) - BoundT(lhv.caster(), 1);
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
        return IntervalT::getBottom(lhv.caster());
    } else {
        auto&& shift = rhv.meet(IntervalT::interval(BoundT(lhv.caster(), 0), BoundT::plusInfinity(lhv.caster())));

        if (shift->isBottom()) {
            return IntervalT::getBottom(lhv.caster());
        }

        auto* shiftRaw = llvm::cast<IntervalT>(shift.get());

        auto coeff = IntervalT(
                BoundT(lhv.caster(), 1) << shiftRaw->lb(),
                shiftRaw->ub().isFinite() ? (BoundT(lhv.caster(), 1) << shiftRaw->ub()) : BoundT::plusInfinity(lhv.caster())
        );
        return lhv * coeff;
    }
}

template <typename Number>
Interval<Number> operator>>(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom(lhv.caster());
    } else {
        auto&& shift = rhv.meet(IntervalT::interval(BoundT(lhv.caster(), 0), BoundT::plusInfinity(lhv.caster())));
        auto* shiftRaw = llvm::cast<IntervalT>(shift.get());

        if (shift->isBottom()) {
            return IntervalT::getBottom(lhv.caster());
        }

        if (lhv.contains(0)) {
            return IntervalT::getTop(lhv.caster());
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
        return IntervalT::getBottom(lhv.caster());
    } else {
        auto&& shift = rhv.meet(IntervalT::interval(BoundT(lhv.caster(), 0), BoundT::plusInfinity(lhv.caster())));
        auto* shiftRaw = llvm::cast<IntervalT>(shift.get());

        if (shift->isBottom()) {
            return IntervalT::getBottom(lhv.caster());
        }

        if (lhv.contains(0)) {
            return IntervalT::getTop(lhv.caster());
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
        return IntervalT::getBottom(lhv.caster());
    } else {

        if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() & rhv.asConstant(), lhv.caster());
        } else if (lhv.isZero() || rhv.isZero()) {
            return IntervalT(0, lhv.caster());
        } else if (lhv.isConstant(-1)) {
            return rhv;
        } else if (rhv.isConstant(-1)) {
            return lhv;
        } else {
            auto&& zero = BoundT(lhv.caster(), 0);
            if (lhv.lb() >= zero && rhv.lb() >= zero) {
                return IntervalT(zero, util::min(lhv.ub(), rhv.ub()));
            } else if (lhv.lb() >= zero) {
                return IntervalT(zero, lhv.ub());
            } else if (lhv.lb() >= zero) {
                return IntervalT(zero, rhv.ub());
            } else {
                return IntervalT::getTop(lhv.caster());
            }
        }
    }
}

template <typename Number>
Interval<Number> operator|(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom(lhv.caster());
    } else {

        if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() | rhv.asConstant(), lhv.caster());
        } else if (lhv.isConstant(-1) || rhv.isConstant(-1)) {
            return IntervalT(-1, lhv.caster());
        } else if (rhv.isZero()) {
            return lhv;
        } else if (lhv.isZero()) {
            return rhv;
        } else {
            return IntervalT::getTop(lhv.caster());
        }
    }
}

template <typename Number>
Interval<Number> operator^(const Interval<Number>& lhv, const Interval<Number>& rhv) {
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;

    if (lhv.isBottom() || rhv.isBottom()) {
        return IntervalT::getBottom(lhv.caster());
    } else {

        if (lhv.isConstant() && rhv.isConstant()) {
            return IntervalT(lhv.asConstant() ^ rhv.asConstant(), lhv.caster());
        } else if (rhv.isZero()) {
            return lhv;
        } else if (lhv.isZero()) {
            return rhv;
        } else {
            return IntervalT::getTop(lhv.caster());
        }
    }
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"


#endif //BOREALIS_INTERVAL_HPP

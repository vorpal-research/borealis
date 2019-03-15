//
// Created by abdullin on 1/25/19.
//

#ifndef BOREALIS_BOUND_HPP
#define BOREALIS_BOUND_HPP

#include "Number.hpp"
#include "Util/cache.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {

namespace util {

template <typename Number>
struct Adapter;

};

namespace absint {

template <typename Number>
class Bound {
public:

    using CasterT = util::Adapter<Number>;

private:
    Bound(const CasterT* caster, bool isPInfinite, bool isNInfinite)
        : caster_(caster), isPlusInfinity_(isPInfinite), isMinusInfinity_(isNInfinite), value_((*caster_)(0)) {
        this->normalize();
    }

    void normalize() {
        ASSERTC(not (isPlusInfinity_ && isMinusInfinity_))
        if (isInfinite()) {
            value_ = (*caster_)(0);
        }
    }

public:
    explicit Bound(const CasterT* caster) : Bound(caster, false, false) {}
    Bound(const CasterT* caster, int n) : caster_(caster), isPlusInfinity_(false), isMinusInfinity_(false), value_((*caster_)(n)) {}
    Bound(const CasterT* caster, Number value) : caster_(caster), isPlusInfinity_(false), isMinusInfinity_(false), value_(std::move(value)) {}

    static Bound plusInfinity(const CasterT* caster) { return Bound(caster, true, false); }

    static Bound minusInfinity(const CasterT* caster) { return Bound(caster, false, true); }

    Bound(const Bound&) = default;
    Bound(Bound&&) = default;
    Bound& operator=(const Bound&) = default;
    Bound& operator=(Bound&&) = default;

    Bound& operator=(int n) {
        this->isPlusInfinity_ = false;
        this->isMinusInfinity_ = false;
        this->value_ = (*caster_)(n);
        return *this;
    }

    Bound& operator=(Number n) {
        this->isPlusInfinity_ = false;
        this->isMinusInfinity_ = false;
        this->value_ = std::move(n);
        return *this;
    }

    const CasterT* caster() const { return caster_; }

    bool isInfinite() const { return isPlusInfinity_ || isMinusInfinity_; }

    Number number() const { return value_; }

    bool isFinite() const { return not isInfinite(); }

    bool isPlusInfinity() const { return isPlusInfinity_; }

    bool isMinusInfinity() const { return isMinusInfinity_; }

    bool isZero() const { return isFinite() && value_ == caster_->operator()(0); }

    bool isPositive() const { return value_ > caster_->operator()(0) || isPlusInfinity_; }

    bool isNegative() const { return value_ < caster_->operator()(0) || isMinusInfinity_; }

    explicit operator size_t() const {
        return ((size_t) number());
    }

    Bound& operator++() {
        this->operator+=(Bound(caster_, 1));
        return *this;
    }

    Bound operator-() const {
        if (isPlusInfinity()) return minusInfinity(caster_);
        else if (isMinusInfinity()) return plusInfinity(caster_);
        else return Bound(caster_, -value_);
    }

    void operator+=(const Bound& other) {
        if (this->isFinite() && other.isFinite()) {
            this->value_ += other.value_;
        } else if (this->isFinite() && other.isInfinite()) {
            this->operator=(other);
        } else if (this->isInfinite() && other.isFinite()) {
            return;
        } else if (this->equals(other)) {
            return;
        } else {
            return;
            //UNREACHABLE("undefined operation +oo + -oo");
        }
    }

    void operator-=(const Bound& other) {
        if (this->isFinite() && other.isFinite()) {
            this->value_ -= other.value_;
        } else if (this->isFinite() && other.isInfinite()) {
            this->operator=(-other);
        } else if (this->isInfinite() && other.isFinite()) {
            return;
        } else if (not this->equals(other)) {
            return;
        } else {
            return;
//            UNREACHABLE("undefined operation +oo - +oo");
        }
    }

    void operator*=(const Bound& other) {
        if (this->isFinite() && other.isFinite()) {
            this->value_ *= other.value_;
        } else if (this->isFinite() && other.isInfinite()) {
            this->operator=(other);
        } else if (this->isInfinite() && other.isFinite()) {
            return;
        } else if (this->equals(other)) {
            return;
        } else {
            return;
//            UNREACHABLE("undefined operation +oo * -oo");
        }
    }

    void operator/=(const Bound& other) {
        if (other.isZero()) {
            UNREACHABLE("division by zero");
        } else if (this->isFinite() && other.isFinite()) {
            this->value_ /= other.value_;
        } else if (this->isFinite() && other.isInfinite()) {
            this->operator=(Bound(caster_, 0));
        } else if (this->isInfinite() && other.isFinite()) {
            if (other.value_ >= (*caster_)(0)) {
                return;
            } else {
                this->operator=(this->operator-());
            }
        } else {
            return;
//            UNREACHABLE("undefined operation +oo / -oo");
        }
    }

    void operator%=(const Bound& other) {
        if (other.isZero()) {
            UNREACHABLE("division by zero");
        } else if (this->isFinite() && other.isFinite()) {
            this->value_ %= other.value_;
        } else if (this->isFinite() && other.isInfinite()) {
            return;
        } else if (this->isInfinite() && other.isFinite()) {
            return;
        } else {
            return;
//            UNREACHABLE("undefined operation +oo % -oo");
        }
    }

    bool leq(const Bound& other) const {
        if (other.isPlusInfinity()) {
            return true;
        } else if (this->isPlusInfinity()) {
            return other.isPlusInfinity();
        } else if (other.isMinusInfinity()) {
            return this->isMinusInfinity();
        } else if (this->isMinusInfinity()) {
            return true;
        } else {
            return this->value_ <= other.value_;
        }
    }

    bool geq(const Bound& other) const {
        if (other.isPlusInfinity()) {
            return this->isPlusInfinity();
        } else if (this->isPlusInfinity()) {
            return true;
        } else if (other.isMinusInfinity()) {
            return true;
        } else if (this->isMinusInfinity()) {
            return other.isMinusInfinity();
        } else {
            return this->value_ >= other.value_;
        }
    }

    bool equals(const Bound& other) const {
        return this->isPlusInfinity_ == other.isPlusInfinity_ && this->isMinusInfinity_ == other.isMinusInfinity_ && this->value_ == other.value_;
    }

    size_t hashCode() const {
        return util::hash::defaultHasher()(isPlusInfinity_, isMinusInfinity_, value_);
    }

    std::string toString() const {
        std::stringstream out;
        if (isFinite()) {
            out << number();
        } else {
            if (isPositive()) out << "+";
            else out << "-";
            out << "inf";
        }
        return out.str();
    }

private:

    const CasterT* caster_;
    bool isPlusInfinity_;
    bool isMinusInfinity_;
    Number value_;

};

template <typename T>
Bound<T> operator+(const Bound<T>& lhv, const Bound<T>& rhv) {
    Bound<T> result(lhv);
    result += rhv;
    return result;
}

template <typename T>
Bound<T> operator-(const Bound<T>& lhv, const Bound<T>& rhv) {
    Bound<T> result(lhv);
    result -= rhv;
    return result;
}

template <typename T>
Bound<T> operator*(const Bound<T>& lhv, const Bound<T>& rhv) {
    Bound<T> result(lhv);
    result *= rhv;
    return result;
}

template <typename T>
Bound<T> operator/(const Bound<T>& lhv, const Bound<T>& rhv) {
    Bound<T> result(lhv);
    result /= rhv;
    return result;
}

template <typename T>
Bound<T> operator%(const Bound<T>& lhv, const Bound<T>& rhv) {
    Bound<T> result(lhv);
    result %= rhv;
    return result;
}

template <typename Number>
inline bool operator<=(const Bound<Number>& lhs, const Bound<Number>& rhs) {
    return lhs.leq(rhs);
}

template <typename Number>
inline bool operator>=(const Bound<Number>& lhs, const Bound<Number>& rhs) {
    return lhs.geq(rhs);
}

template <typename Number>
inline bool operator>=(const Bound<Number>& lhs, int n) {
    return lhs.geq(Bound<Number>(lhs.caster(), n));
}

template <typename Number>
inline bool operator<(const Bound<Number>& lhs, const Bound<Number>& rhs) {
    return !lhs.geq(rhs);
}

template <typename Number>
inline bool operator>(const Bound<Number>& lhs, const Bound<Number>& rhs) {
    return !lhs.leq(rhs);
}

template <typename Number>
inline bool operator==(const Bound<Number>& lhs, const Bound<Number>& rhs) {
    return lhs.equals(rhs);
}

template <typename Number>
inline bool operator!=(const Bound<Number>& lhs, const Bound<Number>& rhs) {
    return !lhs.equals(rhs);
}

template <typename Number>
inline Bound<Number> abs(const Bound<Number>& b) {
    return (b >= 0) ? b : -b;
}

template <typename Number>
Bound<Number> operator<<(const Bound<Number>& lhv, const Bound<Number>& rhv) {
    using BoundT = Bound<Number>;
    ASSERT(rhv >= 0, "rhv of shift should be positive");

    if (lhv.isZero()) {
        return lhv;
    } else if (lhv.isInfinite()) {
        return lhv;
    } else if (rhv.isInfinite()) {
        return (lhv.isPositive() ? BoundT::plusInfinity(lhv.caster()) : BoundT::minusInfinity(lhv.caster()));
    } else {
        return BoundT(lhv.caster(), lhv.number() << rhv.number());
    }
}

template <typename Number>
Bound<Number> operator>>(const Bound<Number>& lhv, const Bound<Number>& rhv) {
    using BoundT = Bound<Number>;
    ASSERT(rhv >= 0, "rhv of shift should be positive");

    if (lhv.isZero()) {
        return lhv;
    } else if (lhv.isInfinite()) {
        return lhv;
    } else if (rhv.isInfinite()) {
        return lhv.isPositive() ? BoundT::plusInfinity(lhv.caster()) : BoundT::minusInfinity(lhv.caster());
    } else {
        return BoundT(lhv.caster(), lhv.number() >> rhv.number());
    }
}

template <typename Number>
Bound<Number> lshr(const Bound<Number>& lhv, const Bound<Number>& rhv) {
    using BoundT = Bound<Number>;
    ASSERT(rhv >= 0, "rhv of shift should be positive");

    if (lhv.isZero()) {
        return lhv;
    } else if (lhv.isInfinite()) {
        return lhv;
    } else if (rhv.isInfinite()) {
        return BoundT(lhv.caster(), 0);
    } else {
        return BoundT(lhv.caster(), lhv.number() >> rhv.number());
    }
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const Bound<T>& bound) {
    if (bound.isFinite()) {
        out << bound.number();
    } else {
        if (bound.isPositive()) out << "+";
        else out << "-";
        out << "inf";
    }
    return out;
}

} // namespace absint

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace util {

template <typename Number>
struct Adapter {
    Number operator()(int) const {
        UNREACHABLE("Unimplemented");
    }

    Number operator()(long) const {
        UNREACHABLE("Unimplemented");
    }

    Number operator()(double) const {
        UNREACHABLE("Unimplemented");
    }
};

template <bool sign>
struct Adapter<absint::BitInt<sign>> {
private:
    static util::cache<size_t, Adapter<absint::BitInt<sign>>*> cache_;

private:
    size_t width_;

public:

    static Adapter<absint::BitInt<sign>>* get(size_t width) {
        return cache_[width];
    }

    explicit Adapter(size_t w) : width_(w) {}

    Adapter(const Adapter&) = default;

    Adapter(Adapter&&) = default;

    Adapter& operator=(const Adapter&) = default;

    Adapter& operator=(Adapter&&) = default;

    size_t width() const { return width_; }

    absint::BitInt<sign> operator()(int n) const {
        return absint::BitInt<sign>(width_, n);
    }

    absint::BitInt<sign> operator()(long n) const {
        return absint::BitInt<sign>(width_, n);
    }

    absint::BitInt<sign> operator()(double n) const {
        return absint::BitInt<sign>(width_, (int) n);
    }
};

template <bool sign>
util::cache<size_t, Adapter<absint::BitInt<sign>>*> Adapter<absint::BitInt<sign>>::cache_(
        [](size_t a) -> Adapter<absint::BitInt<sign>>* { return new Adapter<absint::BitInt<sign>>(a); }
);

template <>
struct Adapter<absint::Float> {
    static Adapter<absint::Float>* get() {
        static Adapter<absint::Float>* instance = new Adapter<absint::Float>();
        return instance;
    }

    size_t width() const { return 64; }

    absint::Float operator()(int n) const {
        return absint::Float(n);
    }

    absint::Float operator()(long n) const {
        return absint::Float((double) n);
    }

    absint::Float operator()(double n) const {
        return absint::Float(n);
    }
};

template <>
struct Adapter<size_t> {
    static Adapter<size_t>* get() {
        static Adapter<size_t>* instance = new Adapter<size_t>();
        return instance;
    }

    size_t operator()(int n) const {
        return size_t(n);
    }

    size_t operator()(long n) const {
        return size_t(n);
    }

    size_t operator()(double n) const {
        return size_t(n);
    }
};

} // namespace util
} // namespace borealis

namespace std {

template <typename Number>
struct hash<borealis::absint::Bound<Number>> {
    size_t operator()(const borealis::absint::Bound<Number>& num) {
        return num.hashCode();
    }
};

} // namespace std

#include "Util/unmacros.h"

#endif //BOREALIS_BOUND_HPP

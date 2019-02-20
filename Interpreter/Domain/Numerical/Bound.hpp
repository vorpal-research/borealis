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
    Bound(const CasterT* caster, bool isInfinite, int n) : caster_(caster), isInfinite_(isInfinite), value_((*caster_)(n)) {
        this->normalize();
    }

    Bound(const CasterT* caster, bool isInfinite, Number n) : caster_(caster), isInfinite_(isInfinite), value_(std::move(n)) {
        this->normalize();
    }

    void normalize() {
        if (this->isInfinite_) {
            value_ = (sgeq(value_, (*caster_)(0))) ? (*caster_)(1) : (*caster_)(-1);
        }
    }

public:
    explicit Bound(const CasterT* caster) : caster_(caster), isInfinite_(false), value_(caster_->operator()(0)) {}
    Bound(const CasterT* caster, int n) : caster_(caster), isInfinite_(false), value_(n) {}
    Bound(const CasterT* caster, Number value) : caster_(caster), isInfinite_(false), value_(std::move(value)) {}

    static Bound plusInfinity(const CasterT* caster) { return Bound(caster, true, 1); }

    static Bound minusInfinity(const CasterT* caster) { return Bound(caster, true, -1); }

    Bound(const Bound&) = default;
    Bound(Bound&&) = default;
    Bound& operator=(const Bound&) = default;
    Bound& operator=(Bound&&) = default;

    Bound& operator=(int n) {
        this->isInfinite_ = false;
        this->value_ = (*caster_)(n);
        return *this;
    }

    Bound& operator=(Number n) {
        this->isInfinite_ = false;
        this->value_ = std::move(n);
        return *this;
    }

    const CasterT* caster() const { return caster_; }

    bool isInfinite() const { return isInfinite_; }

    Number number() const { return value_; }

    bool isFinite() const { return not isInfinite(); }

    bool isPlusInfinity() const { return isInfinite() and this->value_ == (*caster_)(1); }

    bool isMinusInfinity() const { return isInfinite() and this->value_ == (*caster_)(-1); }

    bool isZero() const { return this->value_ == caster_->operator()(0); }

    bool isPositive() const { return this->value_ > caster_->operator()(0); }

    bool isNegative() const { return this->value_ < caster_->operator()(0); }

    explicit operator size_t() const {
        return ((size_t) number());
    }

    Bound& operator++() {
        this->operator+=(Bound(caster_, 1));
        return *this;
    }

    Bound operator-() const { return Bound(caster_, this->isInfinite_, -this->value_); }

    void operator+=(const Bound& other) {
        if (this->isFinite() && other.isFinite()) {
            this->value_ += other.value_;
        } else if (this->isFinite() && other.isInfinite()) {
            this->operator=(other);
        } else if (this->isInfinite() && other.isFinite()) {
            return;
        } else if (this->value_ == other.value_) {
            return;
        } else {
            UNREACHABLE("undefined operation +oo + -oo");
        }
    }

    void operator-=(const Bound& other) {
        if (this->isFinite() && other.isFinite()) {
            this->value_ -= other.value_;
        } else if (this->isFinite() && other.isInfinite()) {
            this->operator=(Bound(caster(), true, -other.value_));
        } else if (this->isInfinite() && other.isFinite()) {
            return;
        } else if (this->value_ != other.value_) {
            return;
        } else {
            UNREACHABLE("undefined operation +oo - +oo");
        }
    }

    void operator*=(const Bound& other) {
        if (this->isZero()) {
            return;
        } else if (other.isZero()) {
            this->operator=(other);
        } else {
            this->value_ *= other.value_;
            this->isInfinite_ = (this->isInfinite_ || other.isInfinite_);
            this->normalize();
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
            this->isInfinite_ = true;
            this->value_ *= other.value_;
            this->normalize();
        }
    }

    void operator%=(const Bound& other) {
        if (this->isZero()) {
            this->operator=(other);
        } else if (other.isZero()) {
            UNREACHABLE("division by zero");
        } else {
            this->value_ %= other.value_;
            this->isInfinite_ = (this->isInfinite_ || other.isInfinite_);
            this->normalize();
        }
    }

    bool leq(const Bound& other) const {
        if (this->isInfinite_ xor other.isInfinite_) {
            if (this->isInfinite_) {
                return this->value_ == (*caster_)(-1);
            } else {
                return other.value_ == (*caster_)(1);
            }
        }
        return this->value_ <= other.value_;
    }

    bool geq(const Bound& other) const {
        if (this->isInfinite_ xor other.isInfinite_) {
            if (this->isInfinite_) {
                return this->value_ == (*caster_)(1);
            } else {
                return other.value_ == (*caster_)(-1);
            }
        }
        return this->value_ >= other.value_;
    }

    bool equals(const Bound& other) const {
        return this->isInfinite_ == other.isInfinite_ && this->value_ == other.value_;
    }

    size_t hashCode() const {
        return util::hash::defaultHasher()(isInfinite_, value_);
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
    bool isInfinite_;
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
        return BoundT(lhv.caster(), lhv.number() >= 0 ? 0 : -1);
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

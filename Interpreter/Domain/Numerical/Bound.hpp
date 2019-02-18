//
// Created by abdullin on 1/25/19.
//

#ifndef BOREALIS_BOUND_HPP
#define BOREALIS_BOUND_HPP

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Number>
class Bound {
private:
    Bound(bool isInfinite, int n) : isInfinite_(isInfinite), value_(n) {
        this->normalize();
    }

    Bound(bool isInfinite, Number n) : isInfinite_(isInfinite), value_(std::move(n)) {
        this->normalize();
    }

    void normalize() {
        if (this->isInfinite_) {
            value_ = (value_ >= 0) ? 1 : -1;
        }
    }

public:
    Bound() : isInfinite_(false), value_(0) {}
    explicit Bound(int n) : isInfinite_(false), value_(n) {}
    explicit Bound(Number value) : isInfinite_(false), value_(std::move(value)) {}

    static Bound plusInfinity() { return Bound(true, 1); }

    static Bound minusInfinity() { return Bound(true, -1); }

    Bound(const Bound&) = default;
    Bound(Bound&&) = default;
    Bound& operator=(const Bound&) = default;
    Bound& operator=(Bound&&) = default;

    Bound& operator=(int n) {
        this->isInfinite_ = false;
        this->value_ = n;
        return *this;
    }

    Bound& operator=(Number n) {
        this->isInfinite_ = false;
        this->value_ = std::move(n);
        return *this;
    }

    bool isInfinite() const { return isInfinite_; }

    Number number() const { return value_; }

    bool isFinite() const { return not isInfinite(); }

    bool isPlusInfinity() const { return isInfinite() and this->value_ == 1; }

    bool isMinusInfinity() const { return isInfinite() and this->value_ == -1; }

    bool isZero() const { return this->value_ == 0; }

    bool isPositive() const { return this->value_ > 0; }

    bool isNegative() const { return this->value_ < 0; }

    explicit operator size_t() const {
        return ((size_t) number());
    }

    Bound& operator++() {
        this->operator+=(Bound(1));
        return *this;
    }

    Bound operator-() const { return Bound(this->isInfinite_, -this->value_); }

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
            this->operator=(Bound(true, -other.value_));
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
            this->operator=(Bound<Number>(0));
        } else if (this->isInfinite() && other.isFinite()) {
            if (other.value_ >= 0) {
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
                return this->value_ == (Number) -1;
            } else {
                return other.value_ == 1;
            }
        }
        return this->value_ <= other.value_;
    }

    bool geq(const Bound& other) const {
        if (this->isInfinite_ xor other.isInfinite_) {
            if (this->isInfinite_) {
                return this->value_ == 1;
            } else {
                return other.value_ == (Number) -1;
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
    return (b >= Bound<Number>(0)) ? b : -b;
}

template <typename Number>
Bound<Number> operator<<(const Bound<Number>& lhv, const Bound<Number>& rhv) {
    using BoundT = Bound<Number>;
    ASSERT(rhv.geq(BoundT(0)), "rhv of shift should be positive");

    if (lhv.isZero()) {
        return lhv;
    } else if (lhv.isInfinite()) {
        return lhv;
    } else if (rhv.isInfinite()) {
        return (lhv.isPositive() ? BoundT::plusInfinity() : BoundT::minusInfinity());
    } else {
        return BoundT(lhv.number() << rhv.number());
    }
}

template <typename Number>
Bound<Number> operator>>(const Bound<Number>& lhv, const Bound<Number>& rhv) {
    using BoundT = Bound<Number>;
    ASSERT(rhv.geq(BoundT(0)), "rhv of shift should be positive");

    if (lhv.isZero()) {
        return lhv;
    } else if (lhv.isInfinite()) {
        return lhv;
    } else if (rhv.isInfinite()) {
        return BoundT(lhv.number() >= 0 ? 0 : -1);
    } else {
        return BoundT(lhv.number() >> rhv.number());
    }
}

template <typename Number>
Bound<Number> lshr(const Bound<Number>& lhv, const Bound<Number>& rhv) {
    using BoundT = Bound<Number>;
    ASSERT(rhv.geq(BoundT(0)), "rhv of shift should be positive");

    if (lhv.isZero()) {
        return lhv;
    } else if (lhv.isInfinite()) {
        return lhv;
    } else if (rhv.isInfinite()) {
        return BoundT(0);
    } else {
        return BoundT(lhv.number() >> rhv.number());
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

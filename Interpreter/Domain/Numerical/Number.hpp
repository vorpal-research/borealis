//
// Created by abdullin on 1/28/19.
//

#ifndef BOREALIS_NUMBER_HPP
#define BOREALIS_NUMBER_HPP

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APFloat.h>

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <unsigned int width, bool sign>
class IntNumber;

enum FloatSemantics {
    HALF,
    SINGLE,
    DOUBLE,
    QUAD
};

template <FloatSemantics semantics>
class FloatNumber;

template <unsigned int width, bool sign = true>
class IntNumber {
private:

    using Self = IntNumber<width, sign>;

    explicit IntNumber(llvm::APInt value) : inner_(std::move(value)) {}

    template <FloatSemantics semantics>
    friend class FloatNumber;

public:

    IntNumber() : inner_(width, 0) {}
    explicit IntNumber(int n) : inner_(width, n) {}
    IntNumber(const IntNumber&) = default;
    IntNumber(IntNumber&&) = default;
    IntNumber& operator=(const IntNumber&) = default;
    IntNumber& operator=(IntNumber&&) = default;

    IntNumber& operator=(int n) {
        inner_ = llvm::APInt(width, n);
        return *this;
    }

    IntNumber operator-() const { return Self(-inner_); }

    void operator+=(const Self& other) {
        inner_ += other.inner_;
    }

    void operator-=(const Self& other) {
        inner_ -= other.inner_;
    }

    void operator*=(const Self& other) {
        inner_ *= other.inner_;
    }

    void operator/=(const Self& other) {
        inner_ = (sign ? inner_.sdiv(other.inner_) : inner_.udiv(other.inner_));
    }

    void operator%=(const Self& other) {
        inner_ = (sign ? inner_.srem(other.inner_) : inner_.urem(other.inner_));
    }

    IntNumber lshr(const IntNumber& other) const {
        return Self(this->inner_.lshr(other.inner_));
    }

    bool leq(const IntNumber& other) const {
        return (sign ? this->inner_.sle(other.inner_) : this->inner_.ule(other.inner_));
    }

    bool geq(const IntNumber& other) const {
        return other.leq(*this);
    }

    std::string toString() const {
        return util::toString(inner_, sign);
    }

    template <FloatSemantics semantics>
    FloatNumber<semantics> toFp() const {
        auto newValue = llvm::APFloat(FloatNumber<semantics>::getLlvmSemantics(), this->toString());
        return FloatNumber<semantics>(newValue);
    }

    template <unsigned int newWidth, bool newSign>
    IntNumber<newWidth, newSign> convert() const {
        llvm::APInt newInner = width < newWidth ?
                newSign ? inner_.sext(newWidth) : inner_.zext(newWidth)
                : inner_.trunc(width);
        return IntNumber<newWidth, newSign>(newInner);
    }

private:

    llvm::APInt inner_;

};

template <int width, bool sign>
IntNumber<width, sign> operator+(const IntNumber<width, sign>& lhv, const IntNumber<width, sign>& rhv) {
    IntNumber<width, sign> result(lhv);
    result += rhv;
    return result;
}

template <int width, bool sign>
IntNumber<width, sign> operator-(const IntNumber<width, sign>& lhv, const IntNumber<width, sign>& rhv) {
    IntNumber<width, sign> result(lhv);
    result -= rhv;
    return result;
}

template <int width, bool sign>
IntNumber<width, sign> operator*(const IntNumber<width, sign>& lhv, const IntNumber<width, sign>& rhv) {
    IntNumber<width, sign> result(lhv);
    result *= rhv;
    return result;
}

template <int width, bool sign>
IntNumber<width, sign> operator/(const IntNumber<width, sign>& lhv, const IntNumber<width, sign>& rhv) {
    IntNumber<width, sign> result(lhv);
    result /= rhv;
    return result;
}

template <int width, bool sign>
IntNumber<width, sign> operator%(const IntNumber<width, sign>& lhv, const IntNumber<width, sign>& rhv) {
    IntNumber<width, sign> result(lhv);
    result %= rhv;
    return result;
}

template <int width, bool sign>
inline bool operator<=(const IntNumber<width, sign>& lhs, const IntNumber<width, sign>& rhs) {
    return lhs.leq(rhs);
}

template <int width, bool sign>
inline bool operator>=(const IntNumber<width, sign>& lhs, const IntNumber<width, sign>& rhs) {
    return lhs.geq(rhs);
}

template <int width, bool sign>
inline bool operator<(const IntNumber<width, sign>& lhs, const IntNumber<width, sign>& rhs) {
    return !lhs.geq(rhs);
}

template <int width, bool sign>
inline bool operator>(const IntNumber<width, sign>& lhs, const IntNumber<width, sign>& rhs) {
    return !lhs.leq(rhs);
}

template <int width, bool sign>
inline bool operator==(const IntNumber<width, sign>& lhs, const IntNumber<width, sign>& rhs) {
    return lhs.equals(rhs);
}

template <int width, bool sign>
inline bool operator!=(const IntNumber<width, sign>& lhs, const IntNumber<width, sign>& rhs) {
    return !lhs.equals(rhs);
}

template <int width, bool sign>
inline IntNumber<width, sign> abs(const IntNumber<width, sign>& b) {
    return (b >= IntNumber<width, sign>(0)) ? b : -b;
}

template <int width, bool sign>
IntNumber<width, sign> operator<<(const IntNumber<width, sign>& lhv, const IntNumber<width, sign>& rhv) {
    return lhv << rhv;
}

template <int width, bool sign>
IntNumber<width, sign> operator>>(const IntNumber<width, sign>& lhv, const IntNumber<width, sign>& rhv) {
    return lhv >> rhv;
}

template <int width, bool sign>
IntNumber<width, sign> lshr(const IntNumber<width, sign>& lhv, const IntNumber<width, sign>& rhv) {
    return lhv.lshr(rhv);
}

template <int width, bool sign>
std::ostream& operator<<(std::ostream& out, const IntNumber<width, sign>& num) {
    out << num.toString();
    return out;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <FloatSemantics semantics = DOUBLE>
class FloatNumber {
private:

    using Self = FloatNumber<semantics>;

    static const llvm::fltSemantics& getLlvmSemantics() {
        switch (semantics) {
            case HALF: return llvm::APFloat::IEEEhalf;
            case SINGLE: return llvm::APFloat::IEEEsingle;
            case DOUBLE: return llvm::APFloat::IEEEdouble;
            case QUAD: return llvm::APFloat::IEEEquad;
            default:
                UNREACHABLE("Unknown float semantics");
        }
    }

    static llvm::APFloat::roundingMode getRoundingMode() {
        return llvm::APFloat::rmNearestTiesToEven;
    }

    explicit FloatNumber(llvm::APFloat value) : inner_(std::move(value)) {}

    template <unsigned int width, bool sign>
    friend class IntNumber;

public:

    FloatNumber() : inner_(getLlvmSemantics(), 0.0) {}
    explicit FloatNumber(double n) : inner_(getLlvmSemantics(), n) {}
    FloatNumber(const FloatNumber&) = default;
    FloatNumber(FloatNumber&&) = default;
    FloatNumber& operator=(const FloatNumber&) = default;
    FloatNumber& operator=(FloatNumber&&) = default;

    FloatNumber& operator=(double n) {
        inner_ = llvm::APFloat(getLlvmSemantics(), n);
        return *this;
    }

    FloatNumber operator-() const { return Self(-inner_); }

    void operator+=(const Self& other) {
        inner_ += other.inner_;
    }

    void operator-=(const Self& other) {
        inner_ -= other.inner_;
    }

    void operator*=(const Self& other) {
        inner_ *= other.inner_;
    }

    void operator/=(const Self& other) {
        inner_ /= other.inner_;
    }

    void operator%=(const Self& other) {
        inner_ %= other.inner_;
    }

    bool leq(const FloatNumber& other) const {
        return util::le(this->inner_, other.inner_);
    }

    bool geq(const FloatNumber& other) const {
        return other.leq(*this);
    }

    std::string toString() const {
        return util::toString(inner_);
    }

    template <unsigned int width, bool sign>
    IntNumber<width, sign> toInt() const {
        llvm::APSInt value(width, sign);
        bool isExact;
        inner_.convertToInteger(&value, getRoundingMode(), &isExact);
        return IntNumber<width, sign>(value);
    }

    template <FloatSemantics newSemantics>
    FloatNumber<newSemantics> convert() const {
        using NewFloatT = FloatNumber<newSemantics>;
        bool isExact;
        return NewFloatT(inner_.convert(NewFloatT::getLlvmSemantics(), NewFloatT::getRoundingMode()), &isExact);
    }

private:

    llvm::APFloat inner_;

};


template <FloatSemantics semantics>
FloatNumber<semantics> operator+(const FloatNumber<semantics>& lhv, const FloatNumber<semantics>& rhv) {
    FloatNumber<semantics> result(lhv);
    result += rhv;
    return result;
}

template <FloatSemantics semantics>
FloatNumber<semantics> operator-(const FloatNumber<semantics>& lhv, const FloatNumber<semantics>& rhv) {
    FloatNumber<semantics> result(lhv);
    result -= rhv;
    return result;
}

template <FloatSemantics semantics>
FloatNumber<semantics> operator*(const FloatNumber<semantics>& lhv, const FloatNumber<semantics>& rhv) {
    FloatNumber<semantics> result(lhv);
    result *= rhv;
    return result;
}

template <FloatSemantics semantics>
FloatNumber<semantics> operator/(const FloatNumber<semantics>& lhv, const FloatNumber<semantics>& rhv) {
    FloatNumber<semantics> result(lhv);
    result /= rhv;
    return result;
}

template <FloatSemantics semantics>
FloatNumber<semantics> operator%(const FloatNumber<semantics>& lhv, const FloatNumber<semantics>& rhv) {
    FloatNumber<semantics> result(lhv);
    result %= rhv;
    return result;
}

template <FloatSemantics semantics>
inline bool operator<=(const FloatNumber<semantics>& lhs, const FloatNumber<semantics>& rhs) {
    return lhs.leq(rhs);
}

template <FloatSemantics semantics>
inline bool operator>=(const FloatNumber<semantics>& lhs, const FloatNumber<semantics>& rhs) {
    return lhs.geq(rhs);
}

template <FloatSemantics semantics>
inline bool operator<(const FloatNumber<semantics>& lhs, const FloatNumber<semantics>& rhs) {
    return !lhs.geq(rhs);
}

template <FloatSemantics semantics>
inline bool operator>(const FloatNumber<semantics>& lhs, const FloatNumber<semantics>& rhs) {
    return !lhs.leq(rhs);
}

template <FloatSemantics semantics>
inline bool operator==(const FloatNumber<semantics>& lhs, const FloatNumber<semantics>& rhs) {
    return lhs.equals(rhs);
}

template <FloatSemantics semantics>
inline bool operator!=(const FloatNumber<semantics>& lhs, const FloatNumber<semantics>& rhs) {
    return !lhs.equals(rhs);
}

template <FloatSemantics semantics>
inline FloatNumber<semantics> abs(const FloatNumber<semantics>& b) {
    return (b >= FloatNumber<semantics>(0.0)) ? b : -b;
}

template <FloatSemantics semantics>
FloatNumber<semantics> lshr(const FloatNumber<semantics>& lhv, const FloatNumber<semantics>& rhv) {
    return lhv.lshr(rhv);
}

template <FloatSemantics semantics>
std::ostream& operator<<(std::ostream& out, const FloatNumber<semantics>& num) {
    out << num.toString();
    return out;
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_NUMBER_HPP

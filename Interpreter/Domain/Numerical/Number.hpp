//
// Created by abdullin on 1/28/19.
//

#ifndef BOREALIS_NUMBER_HPP
#define BOREALIS_NUMBER_HPP

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APFloat.h>

#include "Interpreter/Domain/util/Util.hpp"

#include "Util/hash.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <bool sign>
class BitInt;

template <unsigned int width, bool sign>
class Int;

template <bool sign>
class BitInt {
public:

    static const size_t defaultSize = 64;

private:

    using Self = BitInt<sign>;


public:

    explicit BitInt(llvm::APInt value) : inner_(std::move(value)) {}
    BitInt() : inner_(defaultSize, 0) {}
    explicit BitInt(int n) : inner_(defaultSize, n) {}
    BitInt(size_t width, unsigned long long n) : inner_(width, n) {}
    BitInt(const BitInt&) = default;
    BitInt(BitInt&&) = default;
    BitInt& operator=(const BitInt&) = default;
    BitInt& operator=(BitInt&&) = default;

    size_t width() const { return inner_.getBitWidth(); }

    BitInt& operator=(int n) {
        inner_ = llvm::APInt(defaultSize, n);
        return *this;
    }

    BitInt& operator=(long n) {
        inner_ = llvm::APInt(defaultSize, n);
        return *this;
    }

    explicit operator size_t() const {
        return ((size_t) *inner_.getRawData());
    }

    Self operator-() const { return Self(-inner_); }

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

    Self lshr(const Self& other) const {
        return Self(this->inner_.lshr(other.inner_));
    }

    Self operator&(const Self& rhv) {
        return Self(this->inner_ & rhv.inner_);
    }

    Self operator|(const Self& rhv) {
        return Self(this->inner_ | rhv.inner_);
    }

    Self operator^(const Self& rhv) {
        return Self(this->inner_ ^ rhv.inner_);
    }

    BitInt<sign> operator<<(const BitInt<sign>& rhv) {
        return Self(this->inner_.shl(rhv.inner_));
    }

    BitInt<sign> operator>>(const BitInt<sign>& rhv) {
        return Self(this->inner_.ashr(rhv.inner_));
    }

    bool equals(const Self& other) const {
        return this->inner_ == other.inner_;
    }

    bool leq(const Self& other) const {
        return (sign ? this->inner_.sle(other.inner_) : this->inner_.ule(other.inner_));
    }

    bool sleq(const Self& other) const {
        return this->inner_.sle(other.inner_);
    }

    bool geq(const Self& other) const {
        return other.leq(*this);
    }

    bool sgeq(const Self& other) const {
        return other.sleq(*this);
    }

    BitInt<not sign> changeSignedness() const {
        return BitInt<not sign>(this->inner_);
    }

    std::string toString() const {
        return util::toString(inner_, sign);
    }

    size_t hashCode() const {
        return util::hash::defaultHasher()(inner_);
    }

    template <unsigned int newWidth, bool newSign>
    Int<newWidth, newSign> convert() const {
        llvm::APInt newInner = width() < newWidth ?
                               newSign ? inner_.sext(newWidth) : inner_.zext(newWidth)
                                                : inner_.trunc(width());
        return Int<newWidth, newSign>(newInner);
    }

    template <bool newSign>
    BitInt<newSign> convert(unsigned int newWidth) const {
        llvm::APInt newInner;
        if (width() == newWidth) {
            newInner = inner_;
        } else if (width() < newWidth) {
            newInner = newSign ? inner_.sext(newWidth) : inner_.zext(newWidth);
        } else {
            newInner = inner_.trunc(newWidth);
        }
        return BitInt<newSign>(newInner);
    }

private:

    llvm::APInt inner_;

};

template <bool sign>
BitInt<sign> operator+(const BitInt<sign>& lhv, const BitInt<sign>& rhv) {
    BitInt<sign> result(lhv);
    result += rhv;
    return result;
}

template <bool sign>
BitInt<sign> operator-(const BitInt<sign>& lhv, const BitInt<sign>& rhv) {
    BitInt<sign> result(lhv);
    result -= rhv;
    return result;
}

template <bool sign>
BitInt<sign> operator*(const BitInt<sign>& lhv, const BitInt<sign>& rhv) {
    BitInt<sign> result(lhv);
    result *= rhv;
    return result;
}

template <bool sign>
BitInt<sign> operator/(const BitInt<sign>& lhv, const BitInt<sign>& rhv) {
    BitInt<sign> result(lhv);
    result /= rhv;
    return result;
}

template <bool sign>
BitInt<sign> operator%(const BitInt<sign>& lhv, const BitInt<sign>& rhv) {
    BitInt<sign> result(lhv);
    result %= rhv;
    return result;
}

template <bool sign>
inline bool operator<=(const BitInt<sign>& lhs, const BitInt<sign>& rhs) {
    return lhs.leq(rhs);
}

template <bool sign>
inline bool operator>=(const BitInt<sign>& lhs, const BitInt<sign>& rhs) {
    return lhs.geq(rhs);
}
//
//template <bool sign>
//inline bool operator>=(const BitInt<sign>& lhs, int rhs) {
//    return lhs.sgeq((BitInt<sign>(rhs)));
//}

template <bool sign>
inline bool operator<(const BitInt<sign>& lhs, const BitInt<sign>& rhs) {
    return !lhs.geq(rhs);
}

template <bool sign>
inline bool operator<(const BitInt<sign>& lhs, int rhs) {
    return !lhs.sgeq(BitInt<sign>(lhs.width(), rhs));
}

template <bool sign>
inline bool operator>(const BitInt<sign>& lhs, const BitInt<sign>& rhs) {
    return !lhs.leq(rhs);
}

template <bool sign>
inline bool operator>(const BitInt<sign>& lhs, int rhs) {
    return !lhs.sleq(BitInt<sign>(lhs.width(), rhs));
}

template <bool sign>
inline bool operator==(const BitInt<sign>& lhs, const BitInt<sign>& rhs) {
    return lhs.equals(rhs);
}

template <bool sign>
inline bool operator==(const BitInt<sign>& lhs, int rhs) {
    return lhs.equals(BitInt<sign>(lhs.width(), rhs));
}

template <bool sign>
inline bool operator!=(const BitInt<sign>& lhs, const BitInt<sign>& rhs) {
    return !lhs.equals(rhs);
}

template <bool sign>
inline BitInt<sign> abs(const BitInt<sign>& b) {
    return (b >= BitInt<sign>(0)) ? b : -b;
}

template <bool sign>
BitInt<sign> lshr(const BitInt<sign>& lhv, const BitInt<sign>& rhv) {
    return lhv.lshr(rhv);
}

template <bool sign>
inline bool sgeq(const BitInt<sign>& lhv, const BitInt<sign>& rhv) {
    return lhv.sgeq(rhv);
}

template <bool sign>
std::ostream& operator<<(std::ostream& out, const BitInt<sign>& num) {
    out << num.toString();
    return out;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <unsigned int width, bool sign = true>
class Int {
private:

    using Self = Int<width, sign>;

    friend class Float;

public:

    explicit Int(llvm::APInt value) : inner_(std::move(value)) {}
    Int() : inner_(width, 0) {}
    explicit Int(int n) : inner_(width, n) {}
    Int(const Int&) = default;
    Int(Int&&) = default;
    Int& operator=(const Int&) = default;
    Int& operator=(Int&&) = default;

    Int& operator=(int n) {
        inner_ = llvm::APInt(width, n);
        return *this;
    }

    explicit operator size_t() const {
        return ((size_t) *inner_.getRawData());
    }

    Int operator-() const { return Self(-inner_); }

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

    Int lshr(const Int& other) const {
        return Self(this->inner_.lshr(other.inner_));
    }

    Self operator&(const Self& rhv) {
        return Self(this->inner_ & rhv.inner_);
    }

    Self operator|(const Self& rhv) {
        return Self(this->inner_ | rhv.inner_);
    }

    Self operator^(const Self& rhv) {
        return Self(this->inner_ ^ rhv.inner_);
    }

    Int<width, sign> operator>>(const Int<width, sign>& rhv) {
        return Self(this->inner_.ashr(rhv.inner_));
    }

    bool equals(const Int& other) const {
        return this->inner_ == other.inner_;
    }

    bool leq(const Int& other) const {
        return (sign ? this->inner_.sle(other.inner_) : this->inner_.ule(other.inner_));
    }

    bool geq(const Int& other) const {
        return other.leq(*this);
    }

    Int<width, not sign> changeSignedness() const {
        return Int<width, not sign>(this->inner_);
    }

    std::string toString() const {
        return util::toString(inner_, sign);
    }

    size_t hashCode() const {
        return util::hash::defaultHasher()(inner_);
    }

    template <unsigned int newWidth, bool newSign>
    Int<newWidth, newSign> convert() const {
        llvm::APInt newInner = width < newWidth ?
                newSign ? inner_.sext(newWidth) : inner_.zext(newWidth)
                : inner_.trunc(width);
        return Int<newWidth, newSign>(newInner);
    }

private:

    llvm::APInt inner_;

};

template <unsigned int width, bool sign>
Int<width, sign> operator+(const Int<width, sign>& lhv, const Int<width, sign>& rhv) {
    Int<width, sign> result(lhv);
    result += rhv;
    return result;
}

template <unsigned int width, bool sign>
Int<width, sign> operator-(const Int<width, sign>& lhv, const Int<width, sign>& rhv) {
    Int<width, sign> result(lhv);
    result -= rhv;
    return result;
}

template <unsigned int width, bool sign>
Int<width, sign> operator*(const Int<width, sign>& lhv, const Int<width, sign>& rhv) {
    Int<width, sign> result(lhv);
    result *= rhv;
    return result;
}

template <unsigned int width, bool sign>
Int<width, sign> operator/(const Int<width, sign>& lhv, const Int<width, sign>& rhv) {
    Int<width, sign> result(lhv);
    result /= rhv;
    return result;
}

template <unsigned int width, bool sign>
Int<width, sign> operator%(const Int<width, sign>& lhv, const Int<width, sign>& rhv) {
    Int<width, sign> result(lhv);
    result %= rhv;
    return result;
}

template <unsigned int width, bool sign>
inline bool operator<=(const Int<width, sign>& lhs, const Int<width, sign>& rhs) {
    return lhs.leq(rhs);
}

template <unsigned int width, bool sign>
inline bool operator>=(const Int<width, sign>& lhs, const Int<width, sign>& rhs) {
    return lhs.geq(rhs);
}

template <unsigned int width, bool sign>
inline bool operator>=(const Int<width, sign>& lhs, int rhs) {
    return lhs.geq(Int<width, sign>(rhs));
}

template <unsigned int width, bool sign>
inline bool operator<(const Int<width, sign>& lhs, const Int<width, sign>& rhs) {
    return !lhs.geq(rhs);
}

template <unsigned int width, bool sign>
inline bool operator<(const Int<width, sign>& lhs, int rhs) {
    return !lhs.geq(Int<width, sign>(rhs));
}

template <unsigned int width, bool sign>
inline bool operator>(const Int<width, sign>& lhs, const Int<width, sign>& rhs) {
    return !lhs.leq(rhs);
}

template <unsigned int width, bool sign>
inline bool operator>(const Int<width, sign>& lhs, int rhs) {
    return !lhs.leq(Int<width, sign>(rhs));
}

template <unsigned int width, bool sign>
inline bool operator==(const Int<width, sign>& lhs, const Int<width, sign>& rhs) {
    return lhs.equals(rhs);
}

template <unsigned int width, bool sign>
inline bool operator==(const Int<width, sign>& lhs, int rhs) {
    return lhs.equals(Int<width, sign>(rhs));
}

template <unsigned int width, bool sign>
inline bool operator!=(const Int<width, sign>& lhs, const Int<width, sign>& rhs) {
    return !lhs.equals(rhs);
}

template <unsigned int width, bool sign>
inline Int<width, sign> abs(const Int<width, sign>& b) {
    return (b >= Int<width, sign>(0)) ? b : -b;
}

template <unsigned int width, bool sign>
Int<width, sign> operator<<(const Int<width, sign>& lhv, const Int<width, sign>& rhv) {
    return lhv << rhv;
}

template <unsigned int width, bool sign>
Int<width, sign> lshr(const Int<width, sign>& lhv, const Int<width, sign>& rhv) {
    return lhv.lshr(rhv);
}

template <unsigned int width, bool sign>
std::ostream& operator<<(std::ostream& out, const Int<width, sign>& num) {
    out << num.toString();
    return out;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class Float {
public:

    using Self = Float;

    static const llvm::fltSemantics& getLlvmSemantics() {
        return llvm::APFloat::IEEEdouble;
    }

    static llvm::APFloat::roundingMode getRoundingMode() {
        return llvm::APFloat::rmNearestTiesToEven;
    }

    template <unsigned int width, bool sign>
    friend class Int;

public:
    explicit Float(llvm::APFloat value) : inner_(std::move(value)) {}

    Float() : inner_(getLlvmSemantics(), 0.0) {}
    explicit Float(int n) : inner_(getLlvmSemantics(), n) {}
    explicit Float(double n) : inner_(getLlvmSemantics(), n) {}
    Float(const Float&) = default;
    Float(Float&&) = default;
    Float& operator=(const Float&) = default;
    Float& operator=(Float&&) = default;

    Float& operator=(double n) {
        inner_ = llvm::APFloat(getLlvmSemantics(), n);
        return *this;
    }

    explicit operator size_t() const {
        UNREACHABLE("Unsupported");
    }

    Float operator-() const {
        auto neg = llvm::APFloat(inner_);
        neg.changeSign();
        return Self(neg);
    }

    void operator+=(const Self& other) {
        inner_.add(other.inner_, getRoundingMode());
    }

    void operator-=(const Self& other) {
        inner_.subtract(other.inner_, getRoundingMode());
    }

    void operator*=(const Self& other) {
        inner_.multiply(other.inner_, getRoundingMode());
    }

    void operator/=(const Self& other) {
        inner_.divide(other.inner_, getRoundingMode());
    }

    void operator%=(const Self&) {
        UNREACHABLE("UNSUPPORTED OPERATION");
    }

    Self operator&(const Self&) {
        UNREACHABLE("UNSUPPORTED OPERATION");
    }

    Self operator|(const Self&) {
        UNREACHABLE("UNSUPPORTED OPERATION");
    }

    Self operator^(const Self&) {
        UNREACHABLE("UNSUPPORTED OPERATION");
    }

    Self operator<<(const Self&) {
        UNREACHABLE("UNSUPPORTED OPERATION");
    }

    Self operator>>(const Self&) {
        UNREACHABLE("UNSUPPORTED OPERATION");
    }

    Self lshr(const Self&) const {
        UNREACHABLE("UNSUPPORTED OPERATION");
    }

    bool equals(const Self& other) const {
        return util::eq(this->inner_, other.inner_);
    }

    bool leq(const Float& other) const {
        return util::le(this->inner_, other.inner_);
    }

    bool geq(const Float& other) const {
        return other.leq(*this);
    }

    std::string toString() const {
        return util::toString(inner_);
    }

    size_t hashCode() const {
        return util::hash::defaultHasher()(inner_);
    }

    template <unsigned int width, bool sign>
    Int<width, sign> toInt() const {
        llvm::APSInt value(width, sign);
        bool isExact;
        inner_.convertToInteger(value, getRoundingMode(), &isExact);
        return Int<width, sign>(value);
    }

    template <bool sign>
    BitInt<sign> toBitInt() const {
        llvm::APSInt value(BitInt<sign>::defaultSize, sign);
        bool isExact;
        inner_.convertToInteger(value, getRoundingMode(), &isExact);
        return BitInt<sign>(value);
    }

private:

    llvm::APFloat inner_;

};


inline Float operator+(const Float& lhv, const Float& rhv) {
    Float result(lhv);
    result += rhv;
    return result;
}

inline Float operator-(const Float& lhv, const Float& rhv) {
    Float result(lhv);
    result -= rhv;
    return result;
}

inline Float operator*(const Float& lhv, const Float& rhv) {
    Float result(lhv);
    result *= rhv;
    return result;
}

inline Float operator/(const Float& lhv, const Float& rhv) {
    Float result(lhv);
    result /= rhv;
    return result;
}

inline Float operator%(const Float& lhv, const Float& rhv) {
    Float result(lhv);
    result %= rhv;
    return result;
}

inline bool operator<=(const Float& lhs, const Float& rhs) {
    return lhs.leq(rhs);
}

inline bool operator<=(const Float& lhs, int rhs) {
    return lhs.leq(Float(rhs));
}

inline bool operator<=(const Float& lhs, double rhs) {
    return lhs.leq(Float(rhs));
}

inline bool operator>=(const Float& lhs, const Float& rhs) {
    return lhs.geq(rhs);
}

inline bool operator>=(const Float& lhs, int rhs) {
    return lhs.geq(Float(rhs));
}

inline bool operator>=(const Float& lhs, double rhs) {
    return lhs.geq(Float(rhs));
}

inline bool operator<(const Float& lhs, const Float& rhs) {
    return !lhs.geq(rhs);
}

inline bool operator<(const Float& lhs, int rhs) {
    return !lhs.geq(Float(rhs));
}

inline bool operator<(const Float& lhs, double rhs) {
    return !lhs.geq(Float(rhs));
}

inline bool operator>(const Float& lhs, const Float& rhs) {
    return !lhs.leq(rhs);
}

inline bool operator>(const Float& lhs, int rhs) {
    return !lhs.leq(Float(rhs));
}

inline bool operator>(const Float& lhs, double rhs) {
    return !lhs.leq(Float(rhs));
}

inline bool operator==(const Float& lhs, const Float& rhs) {
    return lhs.equals(rhs);
}

inline bool operator==(const Float& lhs, int rhs) {
    return lhs.equals(Float(rhs));
}

inline bool operator==(const Float& lhs, double rhs) {
    return lhs.equals(Float(rhs));
}

inline bool operator!=(const Float& lhs, const Float& rhs) {
    return !lhs.equals(rhs);
}

inline Float abs(const Float& b) {
    return (b >= Float(0.0)) ? b : -b;
}

inline Float lshr(const Float& lhv, const Float& rhv) {
    return lhv.lshr(rhv);
}

inline bool sgeq(const Float& lhv, const Float& rhv) {
    return lhv >= rhv;
}

inline bool sgeq(size_t lhv, size_t rhv) {
    return lhv >= rhv;
}

inline std::ostream& operator<<(std::ostream& out, const Float& num) {
    out << num.toString();
    return out;
}

} // namespace absint
} // namespace borealis


namespace std {

template <bool sign>
struct hash<borealis::absint::BitInt<sign>> {
    size_t operator()(const borealis::absint::BitInt<sign>& num) {
        return num.hashCode();
    }
};

template <unsigned int width, bool sign>
struct hash<borealis::absint::Int<width, sign>> {
    size_t operator()(const borealis::absint::Int<width, sign>& num) {
        return num.hashCode();
    }
};

template <>
struct hash<borealis::absint::Float> {
    size_t operator()(const borealis::absint::Float& num) {
        return num.hashCode();
    }
};

} // namespace std

#include "Util/unmacros.h"

#endif //BOREALIS_NUMBER_HPP

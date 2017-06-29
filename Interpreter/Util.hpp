//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_UTILS_HPP
#define BOREALIS_UTILS_HPP

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/IR/Type.h>

#include "Interpreter/Domain/Integer/Integer.h"

namespace borealis {
namespace util {

llvm::APFloat getMaxValue(const llvm::fltSemantics& semantics);
llvm::APFloat getMinValue(const llvm::fltSemantics& semantics);

bool lt(const llvm::APFloat& lhv, const llvm::APFloat& rhv);
bool eq(const llvm::APFloat& lhv, const llvm::APFloat& rhv);
bool gt(const llvm::APFloat& lhv, const llvm::APFloat& rhv);

static bool operator<(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return lt(lhv, rhv);
}

static bool operator==(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return eq(lhv, rhv);
}

static bool operator>(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return gt(lhv, rhv);
}

const llvm::fltSemantics& getSemantics(const llvm::Type& type);

std::string toString(const llvm::APFloat& val);
std::string toString(const llvm::APInt& val, bool isSigned = false);

///////////////////////////////////////////////////////////////
/// templates
///////////////////////////////////////////////////////////////
template <typename T>
T min(const T& t) {
    return t;
}

template <bool isSigned, typename T>
T min(const T& t) {
    return t;
}

template <typename Head, typename ...Tail>
Head min(const Head& h, const Tail&... t) {
    auto tmin = min<Tail...>(t...);
    return h < tmin ? h : tmin;
};

template <bool isSigned, typename Head, typename ...Tail>
Head min(const Head& h, const Tail&... t) {
    auto tmin = min<isSigned, Tail...>(t...);
    return isSigned ?
           (h->slt(tmin) ? h : tmin) :
           (h < tmin ? h : tmin);
};

template <typename T>
T max(const T& t) {
    return t;
}

template <bool isSigned, typename T>
T max(const T& t) {
    return t;
}

template <typename Head, typename ...Tail>
Head max(const Head& h, const Tail&... t) {
    auto tmax = max<Tail...>(t...);
    return h < tmax ? tmax : h;
};

template <bool isSigned, typename Head, typename ...Tail>
Head max(const Head& h, const Tail&... t) {
    auto tmax = max<isSigned, Tail...>(t...);
    return isSigned ?
           (h->slt(tmax) ? tmax : h) :
           (h < tmax ? tmax : h);
};

}   /* namespace util */
}   /* namespace borealis */

///////////////////////////////////////////////////////////////
/// Hashes for APInt and APFloat
///////////////////////////////////////////////////////////////

namespace std {

template <>
struct hash<llvm::APFloat> {
    size_t operator() (const llvm::APFloat& apFloat) const noexcept {
        return llvm::hash_value(apFloat);
    }
};

template <>
struct hash<llvm::APInt> {
    size_t operator() (const llvm::APInt& apInt) const noexcept {
        return llvm::hash_value(apInt);
    }
};

}   /* namespace std */

#endif //BOREALIS_UTILS_HPP

//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_UTILS_HPP
#define BOREALIS_UTILS_HPP

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APSInt.h>
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
bool le(const llvm::APFloat& lhv, const llvm::APFloat& rhv);
bool gt(const llvm::APFloat& lhv, const llvm::APFloat& rhv);
bool ge(const llvm::APFloat& lhv, const llvm::APFloat& rhv);

static bool operator<(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return lt(lhv, rhv);
}

static bool operator==(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return eq(lhv, rhv);
}

static bool operator>(const llvm::APFloat& lhv, const llvm::APFloat& rhv) {
    return gt(lhv, rhv);
}

const llvm::fltSemantics& getSemantics();
const llvm::fltSemantics& getSemantics(const llvm::Type& type);

std::string toString(const llvm::APFloat& val);
std::string toString(const llvm::APInt& val, bool isSigned = false);

///////////////////////////////////////////////////////////////
/// templates
///////////////////////////////////////////////////////////////
template <typename T>
T signed_min(const T& t) {
    return t;
}

template <typename Head, typename ...Tail>
Head signed_min(const Head& h, const Tail&... t) {
    auto tmin = signed_min<Tail...>(t...);
    return h->slt(tmin) ? h : tmin;
};

template <typename T>
T signed_max(const T& t) {
    return t;
}

template <typename Head, typename ...Tail>
Head signed_max(const Head& h, const Tail&... t) {
    auto tmax = signed_max<Tail...>(t...);
    return h->slt(tmax) ? tmax : h;
};

///////////////////////////////////////////////////////////////
/// other
///////////////////////////////////////////////////////////////
bool llvm_types_eq(const llvm::Type* lhv, const llvm::Type* rhv);

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

template <>
struct hash<llvm::APSInt> {
    size_t operator() (const llvm::APSInt& apsInt) const noexcept {
        return std::hash<llvm::APInt>()(static_cast<const llvm::APInt&>(apsInt));
    }
};

}   /* namespace std */

#endif //BOREALIS_UTILS_HPP

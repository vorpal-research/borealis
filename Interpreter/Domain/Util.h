//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_UTILS_HPP
#define BOREALIS_UTILS_HPP

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/IR/Type.h>

namespace borealis {
namespace util {

///////////////////////////////////////////////////////////////
/// APInt util
///////////////////////////////////////////////////////////////

llvm::APInt getMaxValue(unsigned width, bool isSigned = false);
llvm::APInt getMinValue(unsigned width, bool isSigned = false);
bool isMaxValue(const llvm::APInt& val, bool isSigned = false);
bool isMinValue(const llvm::APInt& val, bool isSigned = false);

llvm::APInt min(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false);
llvm::APInt max(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false);

bool eq(const llvm::APInt& lhv, const llvm::APInt& rhv);
bool lt(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false);
bool le(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false);
bool gt(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false);
bool ge(const llvm::APInt& lhv, const llvm::APInt& rhv, bool isSigned = false);

std::string toString(const llvm::APInt& val, bool isSigned = false);

///////////////////////////////////////////////////////////////
/// APFloat util
///////////////////////////////////////////////////////////////

llvm::APFloat getMaxValue(const llvm::fltSemantics& semantics);
llvm::APFloat getMinValue(const llvm::fltSemantics& semantics);

llvm::APFloat min(const llvm::APFloat& lhv, const llvm::APFloat& rhv);
llvm::APFloat max(const llvm::APFloat& lhv, const llvm::APFloat& rhv);

bool lt(const llvm::APFloat& lhv, const llvm::APFloat& rhv);
bool eq(const llvm::APFloat& lhv, const llvm::APFloat& rhv);
bool gt(const llvm::APFloat& lhv, const llvm::APFloat& rhv);

const llvm::fltSemantics& getSemantics(const llvm::Type& type);

std::string toString(const llvm::APFloat& val);

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

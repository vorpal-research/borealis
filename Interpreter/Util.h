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
///////////////////////////////////////////////////////////////
/// APInt util
///////////////////////////////////////////////////////////////

using namespace borealis::absint;

absint::Integer::Ptr getMaxValue(unsigned width);
absint::Integer::Ptr getMinValue(unsigned width);

absint::Integer::Ptr min(absint::Integer::Ptr lhv, absint::Integer::Ptr rhv, bool isSigned = false);
absint::Integer::Ptr max(absint::Integer::Ptr lhv, absint::Integer::Ptr rhv, bool isSigned = false);

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

///////////////////////////////////////////////////////////////
/// Other
///////////////////////////////////////////////////////////////

std::string toString(const llvm::Type& type);

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

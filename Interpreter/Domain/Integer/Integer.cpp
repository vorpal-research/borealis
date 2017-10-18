//
// Created by abdullin on 6/2/17.
//

#include <Type/Integer.h>
#include "Integer.h"
#include "IntMin.h"
#include "IntMax.h"
#include "IntValue.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Integer::Ptr Integer::getMaxValue(Type::Ptr type) {
    auto intType = llvm::dyn_cast<type::Integer>(type.get());
    ASSERT(intType, "Non-integer type in Integer");
    return getMaxValue(intType->getBitsize());
}

Integer::Ptr Integer::getMinValue(Type::Ptr type) {
    auto intType = llvm::dyn_cast<type::Integer>(type.get());
    ASSERT(intType, "Non-integer type in Integer");
    return getMinValue(intType->getBitsize());
}

Integer::Ptr Integer::getValue(uint64_t value, Type::Ptr type) {
    auto intType = llvm::dyn_cast<type::Integer>(type.get());
    ASSERT(intType, "Non-integer type in Integer");
    return getValue(value, intType->getBitsize());
}

Integer::Ptr Integer::getMaxValue(size_t width) {
    return max_cache_[width];
}

Integer::Ptr Integer::getMinValue(size_t width) {
    return min_cache_[width];
}

Integer::Ptr Integer::getValue(uint64_t value, size_t width) {
    return getValue(llvm::APInt(width, value));
}

Integer::Ptr Integer::getValue(const llvm::APInt& value) {
    return val_cache_[value];
}

bool Integer::neq(Integer::Ptr other) const {
    return not eq(other);
}

bool Integer::le(Integer::Ptr other) const {
    return lt(other) || eq(other);
}

bool Integer::gt(Integer::Ptr other) const {
    return not le(other);
}

bool Integer::ge(Integer::Ptr other) const {
    return gt(other) || eq(other);
}

bool Integer::sle(Integer::Ptr other) const {
    return slt(other) || eq(other);
}

bool Integer::sgt(Integer::Ptr other) const {
    return not sle(other);
}

bool Integer::sge(Integer::Ptr other) const {
    return sgt(other) || eq(other);
}

util::cache<size_t, Integer::Ptr> Integer::max_cache_([](auto&& X) -> decltype(auto) { return std::make_shared<IntMax>(X); });
util::cache<size_t, Integer::Ptr> Integer::min_cache_([](auto&& X) -> decltype(auto) { return std::make_shared<IntMin>(X); });
util::cache<llvm::APInt, Integer::Ptr, Integer::CacheImpl> Integer::val_cache_([](auto&& X) -> decltype(auto) {
    return std::make_shared<IntValue>(X);
});

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
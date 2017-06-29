//
// Created by abdullin on 6/2/17.
//

#ifndef BOREALIS_VALINTEGER_H
#define BOREALIS_VALINTEGER_H

#include "Integer.h"

namespace borealis {
namespace absint {

class IntValue : public Integer {
public:

    IntValue(uint64_t value, size_t width);
    IntValue(const llvm::APInt& value, size_t width);

    static bool classof(const Integer* other);
    virtual std::string toString() const;
    virtual size_t hashCode() const;
    const llvm::APInt& getValue() const;

    /// Semantics
    virtual Integer::Ptr add(Integer::Ptr other) const;
    virtual Integer::Ptr sub(Integer::Ptr other) const;
    virtual Integer::Ptr mul(Integer::Ptr other) const;
    virtual Integer::Ptr udiv(Integer::Ptr other) const;
    virtual Integer::Ptr sdiv(Integer::Ptr other) const;
    virtual Integer::Ptr urem(Integer::Ptr other) const;
    virtual Integer::Ptr srem(Integer::Ptr other) const;
    virtual bool eq(Integer::Ptr other) const;
    virtual bool lt(Integer::Ptr other) const;
    virtual bool slt(Integer::Ptr other) const;
    /// Shifts
    virtual Integer::Ptr shl(Integer::Ptr shift) const;
    virtual Integer::Ptr lshr(Integer::Ptr shift) const;
    virtual Integer::Ptr ashr(Integer::Ptr shift) const;
    /// Cast
    virtual Integer::Ptr trunc(const size_t width) const;
    virtual Integer::Ptr zext(const size_t width) const;
    virtual Integer::Ptr sext(const size_t width) const;

private:

    const llvm::APInt value_;

};

}   /* namespace absint */
}   /* namespace borealis */


#endif //BOREALIS_VALINTEGER_H

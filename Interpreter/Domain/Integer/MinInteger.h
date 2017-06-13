//
// Created by abdullin on 6/2/17.
//

#ifndef BOREALIS_MININTEGER_H
#define BOREALIS_MININTEGER_H

#include "Integer.h"

namespace borealis {
namespace absint {

class MinInteger : public Integer {
public:

    MinInteger(size_t width);

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

};

}   /* namespace absint */
}   /* namespace borealis */


#endif //BOREALIS_MININTEGER_H

//
// Created by abdullin on 6/2/17.
//

#ifndef BOREALIS_MAXINTEGER_H
#define BOREALIS_MAXINTEGER_H


#include "Integer.h"

namespace borealis {
namespace absint {

class IntMax : public Integer {
public:

    IntMax(size_t width);

    static bool classof(const Integer* other);
    std::string toString() const override;
    std::string toSignedString() const override;
    size_t hashCode() const override;
    const llvm::APInt& getValue() const override;

    /// Semantics
    Integer::Ptr add(Integer::Ptr other) const override;
    Integer::Ptr sub(Integer::Ptr other) const override;
    Integer::Ptr mul(Integer::Ptr other) const override;
    Integer::Ptr udiv(Integer::Ptr other) const override;
    Integer::Ptr sdiv(Integer::Ptr other) const override;
    Integer::Ptr urem(Integer::Ptr other) const override;
    Integer::Ptr srem(Integer::Ptr other) const override;
    bool eq(Integer::Ptr other) const override;
    bool lt(Integer::Ptr other) const override;
    bool slt(Integer::Ptr other) const override;
    /// Shifts
    Integer::Ptr shl(Integer::Ptr shift) const override;
    Integer::Ptr lshr(Integer::Ptr shift) const override;
    Integer::Ptr ashr(Integer::Ptr shift) const override;
    /// Cast
    Integer::Ptr trunc(const size_t width) const override;
    Integer::Ptr zext(const size_t width) const override;
    Integer::Ptr sext(const size_t width) const override;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_MAXINTEGER_H
//
// Created by abdullin on 6/2/17.
//

#ifndef BOREALIS_INTEGER_H
#define BOREALIS_INTEGER_H

#include <memory>
#include <string>

#include <llvm/IR/Type.h>

namespace borealis {
namespace absint {

class Integer : public std::enable_shared_from_this<const Integer> {
protected:
    enum Type {
        MIN,
        VALUE,
        MAX
    };

public:

    using Ptr = std::shared_ptr<const Integer>;

    Integer(Integer::Type type, size_t width) : type_(type), width_(width) {}
    virtual ~Integer() = default;


    static Integer::Ptr getMaxValue(size_t width);
    static Integer::Ptr getMinValue(size_t width);

    static bool classof(const Integer*) {
        return true;
    }

    virtual bool isMax() const {
        return type_ == MAX;
    }

    virtual bool isValue() const {
        return type_ == VALUE;
    }

    virtual bool isMin() const {
        return type_ == MIN;
    }

    virtual size_t getWidth() const {
        return width_;
    }

    virtual std::string toString() const = 0;
    virtual std::string toSignedString() const = 0;
    virtual size_t hashCode() const = 0;
    virtual const llvm::APInt& getValue() const = 0;
    virtual uint64_t getRawValue() const {
        return *getValue().getRawData();
    }

    /// Comparsion
    virtual bool eq(Integer::Ptr other) const = 0;
    virtual bool lt(Integer::Ptr other) const = 0;
    virtual bool slt(Integer::Ptr other) const = 0;
    virtual bool neq(Integer::Ptr other) const;
    virtual bool le(Integer::Ptr other) const;
    virtual bool gt(Integer::Ptr other) const;
    virtual bool ge(Integer::Ptr other) const;
    virtual bool sle(Integer::Ptr other) const;
    virtual bool sgt(Integer::Ptr other) const;
    virtual bool sge(Integer::Ptr other) const;
    /// Semantics
    virtual Integer::Ptr add(Integer::Ptr other) const = 0;
    virtual Integer::Ptr sub(Integer::Ptr other) const = 0;
    virtual Integer::Ptr mul(Integer::Ptr other) const = 0;
    virtual Integer::Ptr udiv(Integer::Ptr other) const = 0;
    virtual Integer::Ptr sdiv(Integer::Ptr other) const = 0;
    virtual Integer::Ptr urem(Integer::Ptr other) const = 0;
    virtual Integer::Ptr srem(Integer::Ptr other) const = 0;
    /// Shifts
    virtual Integer::Ptr shl(Integer::Ptr shift) const = 0;
    virtual Integer::Ptr lshr(Integer::Ptr shift) const = 0;
    virtual Integer::Ptr ashr(Integer::Ptr shift) const = 0;
    /// Cast
    virtual Integer::Ptr trunc(const size_t width) const = 0;
    virtual Integer::Ptr zext(const size_t width) const = 0;
    virtual Integer::Ptr sext(const size_t width) const = 0;

private:
    Type type_;
    size_t width_;

};

static bool operator<(Integer::Ptr lhv, Integer::Ptr rhv) {
    return lhv->lt(rhv);
}
static bool operator==(Integer::Ptr lhv, Integer::Ptr rhv) {
    return lhv->eq(rhv);
}
static bool operator>(Integer::Ptr lhv, Integer::Ptr rhv) {
    return lhv->gt(rhv);
}

}   /* namespace absint */
}   /* namespace borealis */

namespace std {

template <>
struct hash<borealis::absint::Integer::Ptr> {
    size_t operator()(const borealis::absint::Integer::Ptr& integer) const noexcept {
        return integer->hashCode();
    }
};

}   /* namespace std */
#endif //BOREALIS_INTEGER_H

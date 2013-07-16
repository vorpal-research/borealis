#ifndef BOOL_H
#define BOOL_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class Bool : public Type {
    typedef Bool self;
    typedef Type base;

    Bool() : Type(type_id(*this)) {}

public:
    static bool classof(const self*) { return true; }
    static bool classof(const base* b) { return b->getId() == type_id<self>(); }

    friend class ::borealis::TypeFactory;

};

} // namespace type
} // namespace borealis

#endif // BOOL_H

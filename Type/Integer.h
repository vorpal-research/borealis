#ifndef INTEGER_H
#define INTEGER_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class Integer : public Type {
    typedef Integer self;
    typedef Type base;

    Integer() : Type(type_id(*this)) {}

public:
    static bool classof(const self*) { return true; }
    static bool classof(const base* b) { return b->getId() == type_id<self>(); }

    friend class ::borealis::TypeFactory;
    
};

} // namespace type
} // namespace borealis

#endif // INTEGER_H

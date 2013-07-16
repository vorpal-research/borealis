#ifndef UNKNOWNTYPE_H
#define UNKNOWNTYPE_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class UnknownType : public Type {
    typedef UnknownType self;
    typedef Type base;

    UnknownType() : Type(type_id(*this)) {}

public:
    static bool classof(const self*) { return true; }
    static bool classof(const base* b) { return b->getId() == type_id<self>(); }

    friend class ::borealis::TypeFactory;

};

} // namespace type
} // namespace borealis

#endif // UNKNOWNTYPE_H

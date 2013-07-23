#ifndef UNKNOWNTYPE_H
#define UNKNOWNTYPE_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class UnknownType : public Type {

    typedef UnknownType Self;
    typedef Type Base;

    UnknownType() : Type(class_tag(*this)) {}

public:

    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }
    
    
};

} // namespace type
} // namespace borealis

#endif // UNKNOWNTYPE_H



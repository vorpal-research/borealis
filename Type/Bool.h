#ifndef BOOL_H
#define BOOL_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class Bool : public Type {

    typedef Bool Self;
    typedef Type Base;

    Bool() : Type(class_tag(*this)) {}

public:

    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }
    
    
};

} // namespace type
} // namespace borealis

#endif // BOOL_H



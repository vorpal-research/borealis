#ifndef POINTER_H
#define POINTER_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class Pointer : public Type {

    typedef Pointer Self;
    typedef Type Base;

    Pointer(Type::Ptr pointed) : Type(class_tag(*this)), pointed(pointed) {}

public:

    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }
    
    
private:
    Type::Ptr pointed;
public:
    Type::Ptr getPointed() const { return pointed; }

};

} // namespace type
} // namespace borealis

#endif // POINTER_H



#ifndef FLOAT_H
#define FLOAT_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class Float : public Type {

    typedef Float Self;
    typedef Type Base;

    Float() : Type(class_tag(*this)) {}

public:

    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }
    
    
};

} // namespace type
} // namespace borealis

#endif // FLOAT_H



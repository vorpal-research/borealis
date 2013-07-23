#ifndef INTEGER_H
#define INTEGER_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class Integer : public Type {

    typedef Integer Self;
    typedef Type Base;

    Integer() : Type(class_tag(*this)) {}

public:

    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }
    
    
};

} // namespace type
} // namespace borealis

#endif // INTEGER_H



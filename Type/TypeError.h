#ifndef TYPEERROR_H
#define TYPEERROR_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

class TypeError : public Type {

    typedef TypeError Self;
    typedef Type Base;

    TypeError(const std::string& message) : Type(class_tag(*this)), message(message) {}

public:

    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }
    
    
private:
    std::string message;
public:
    const std::string& getMessage() const { return message; }

};

} // namespace type
} // namespace borealis

#endif // TYPEERROR_H



#ifndef TYPEERROR_H
#define TYPEERROR_H

#include "Type/Type.h"

namespace borealis {

class TypeError : public Type {
    typedef TypeError self;
    typedef Type base;


    TypeError(const std::string& message) : Type(type_id(*this)), message(message) {}

public:
    static bool classof(const self*) { return true; }
    static bool classof(const base* b) { return b->getId() == type_id<self>(); }

    friend class TypeFactory;

    
private:
    std::string message;
public:
    const std::string& getMessage() const { return message; }

};

} // namespace borealis

#endif // TYPEERROR_H



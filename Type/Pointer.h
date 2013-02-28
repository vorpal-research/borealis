#ifndef POINTER_H
#define POINTER_H

#include "Type/Type.h"

namespace borealis {

class Pointer : public Type {
    typedef Pointer self;
    typedef Type base;

    Pointer(Type::Ptr pointed) : Type(type_id(*this)), pointed(pointed) {}

public:
    static bool classof(const self*) { return true; }
    static bool classof(const base* b) { return b->getId() == type_id<self>(); }

    friend class TypeFactory;
    
private:
    Type::Ptr pointed;

public:
    Type::Ptr getPointed() const { return pointed; }

};

} // namespace borealis

#endif // POINTER_H

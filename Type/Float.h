#ifndef FLOAT_H
#define FLOAT_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

/** protobuf -> Type/Float.proto
import "Type/Type.proto";

package borealis.type.proto;

message Float {
    extend borealis.proto.Type {
        optional Float ext = 3;
    }
    
}

**/
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



#ifndef INTEGER_H
#define INTEGER_H

#include "Type/Type.h"

namespace borealis {

class TypeFactory;

namespace type {

/** protobuf -> Type/Type.proto
import "Type/Type.proto";

package borealis.proto.type;

message Integer {
    extend borealis.proto.Type {
        optional Integer obj = 1;
    }
}

**/
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


